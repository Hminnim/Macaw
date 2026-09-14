#include "PCH.h"
#include "FUndoSystem.h"

#include "IUndoContext.h"
#include "FUndoTransaction.h"
#include "FUndoRecords.h" 
#include "FUndoMessages.h"
#include "../../../Serialize/FArchiveMemory.h"
#include "../../Channel/FMessageChannel.h"
#include "../../Channel/FMessage.h"

namespace
{
    template<uint32 Capacity = 64>
    class FUndoHistoryStack
    {
    public:
        FUndoHistoryStack() : Head(0), Size(0), Current(0) {}

        void Push(FUndoTransaction&& NewTransaction)
        {
            Size = Current; 

            if (Size == Capacity)
            {
                Head = (Head + 1) % Capacity;
                --Size;
            }

            uint32 WriteIndex = (Head + Size) % Capacity;
            Buffer[WriteIndex].emplace(std::move(NewTransaction));

            ++Size;
            Current = Size;
        }

        bool CanUndo() const { return Current > 0; }

        FUndoTransaction* Undo()
        {
            if (!CanUndo()) 
                return nullptr;
            --Current;
            uint32 TargetIndex = (Head + Current) % Capacity;
            return &Buffer[TargetIndex].value();
        }

        bool CanRedo() const { return Current < Size; }

        FUndoTransaction* Redo()
        {
            if (!CanRedo()) 
                return nullptr;
            uint32 TargetIndex = (Head + Current) % Capacity;
            ++Current;
            return &Buffer[TargetIndex].value();
        }

        void Clear()
        {
            Head = 0;
            Size = 0;
            Current = 0;
        }

    private:
        uint32 Head;
        uint32 Size;
        uint32 Current;
        TFixedArray<std::optional<FUndoTransaction>, Capacity> Buffer;
    };

    struct FUndoSystemState 
    {
        FUndoHistoryStack<64> History;

        std::unique_ptr<FUndoTransaction> CurrentTransaction = nullptr;
        TArray<std::function<void(FUndoTransaction&)>> PendingFinalizers;
        TSet<UObject*> ModifiedObjectsThisTransaction;

        std::optional<FMessageChannel::FSender> MessageSender;
    };

    static FUndoSystemState& GetState()
    {
        static FUndoSystemState State;
        return State;
    }


    template<typename TMessage> requires CMessageType<std::remove_cvref_t<TMessage>>
    bool SendMessageToWorldChannel(TMessage&& Message)
    {
        FUndoSystemState& State = GetState();
        if (State.MessageSender.has_value())
        {
            return State.MessageSender->TryPush(std::forward<TMessage>(Message));
        }
        return false;
    }
    template<CMessageType TMessage, typename... Args> requires CMessageConstructible<TMessage, Args...>
    bool EmplaceMessageToWorldChannel(Args&&... Arguments)
    {
        FUndoSystemState& State = GetState();
        if (State.MessageSender.has_value())
        {
            return State.MessageSender->TryEmplace<TMessage>(std::forward<Args>(Arguments)...);
        }
        return false;
    }


    class FUndoContextImpl : public IUndoContext
    {
    public:
        virtual void NotifyObjectChanged(const FGuid& Guid, const TArray<uint8>& Data) override
        {
            EmplaceMessageToWorldChannel<FMessageUndoObjectStateChanged>(Guid, TArray<uint8>(Data));
        }

        virtual void NotifyObjectSpawned(const FGuid& Guid, const TArray<uint8>& Data, FString&& TypeName) override
        {
            EmplaceMessageToWorldChannel<FMessageUndoObjectSpawned>(Guid, TArray<uint8>(Data), std::move(TypeName));
        }

        virtual void NotifyObjectDeleted(const FGuid& Guid) override
        {
            EmplaceMessageToWorldChannel<FMessageUndoObjectDestroyed>(FMessageUndoObjectDestroyed(Guid));
        }
    };


}

namespace FUndoSystem
{
    // =================================================================
    // Message Sender 관리 API
    // =================================================================
    void InitializeSenderToWorldChannel(FMessageChannel::FSender&& SenderToWorldChannel)
    {
        FUndoSystemState& State = GetState();
        State.MessageSender.emplace(std::move(SenderToWorldChannel));
    }

    // =================================================================
    // Undo/Redo API
    // =================================================================
    void BeginTransaction(const FString& TransactionName)
    {
        FUndoSystemState& State = GetState();
        if (State.CurrentTransaction != nullptr) 
            return;

        State.CurrentTransaction = std::make_unique<FUndoTransaction>(TransactionName);
    }

    void RecordObject(UObject* TargetObject, EUndoType UndoType, FAssetRegistry* AssetRegistry)
    {
        FUndoSystemState& State = GetState();
        if (!State.CurrentTransaction || !TargetObject)
            return;

        if (UndoType == EUndoType::StateChange)
        {
            if (State.ModifiedObjectsThisTransaction.contains(TargetObject))
                return;

            State.ModifiedObjectsThisTransaction.insert(TargetObject);
        }

        TArray<uint8> CurrentData;
        FArchiveMemory MemoryArchive(CurrentData);
        MemoryArchive.SetAssetRegistry(AssetRegistry);
        TargetObject->Save(MemoryArchive);

        if (UndoType == EUndoType::StateChange)
        {
            State.PendingFinalizers.push_back(
                [TargetObject, CurrentData = std::move(CurrentData)](FUndoTransaction& Transaction)
                {
                    TArray<uint8> AfterData;
                    FArchiveMemory MemoryArchiveAfter(AfterData);
                    TargetObject->Save(MemoryArchiveAfter);

                    auto Record = std::make_unique<FRecordObjectState>(
                        TargetObject->GetGuid(), CurrentData, AfterData);

                    Transaction.AddRecord(std::move(Record));
                }
            );
        }
        else
        {
            std::unique_ptr<IUndoRecord> Record = nullptr;
            switch (UndoType)
            {
                case EUndoType::Spawn:      Record = std::make_unique<FRecordObjectSpawned>(TargetObject->GetGuid(), std::move(CurrentData), TargetObject->GetTypeInfo()->TypeName); break;
                case EUndoType::Destroy:    Record = std::make_unique<FRecordObjectDestroyed>(TargetObject->GetGuid(), std::move(CurrentData), TargetObject->GetTypeInfo()->TypeName); break;
            }

            if (Record)
                State.CurrentTransaction->AddRecord(std::move(Record));
        }
    }


    void EndTransaction()
    {
        FUndoSystemState& State = GetState();
        if (!State.CurrentTransaction)
            return;

        for (const auto& Finalizer : State.PendingFinalizers)
        {
            Finalizer(*State.CurrentTransaction);
        }

        State.History.Push(std::move(*State.CurrentTransaction));

        State.CurrentTransaction.reset();
        State.PendingFinalizers.clear();
        State.ModifiedObjectsThisTransaction.clear();
    }

    void Undo()
    {
        if (FUndoTransaction* Transactions = GetState().History.Undo())
        {
            FUndoContextImpl UndoContext;
            Transactions->Undo(UndoContext);
        }
    }

    void Redo()
    {
        if (FUndoTransaction* Transactions = GetState().History.Redo())
        {
            FUndoContextImpl RedoContext;
            Transactions->Redo(RedoContext);
        }
    }
}
