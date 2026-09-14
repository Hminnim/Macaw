#include "PCH.h"
#include "doctest.h"

#include "../Core/Base/UObject.h"
#include "../Core/Base/UObjectSystem.h"
#include "../Serialize/FArchiveMemory.h"
#include "../Core/Base/UndoSystem/FUndoSystem.h"
#include "../Core/Channel/FMessageChannel.h"
#include "../Core/Base/UndoSystem/FUndoMessages.h"

TEST_SUITE("Undo System Tests")
{
    // =================================================================
    // 테스트용 종합 객체 정의 (Test Object)
    // =================================================================
    class UObjectTest : public UObject
    {
    public:
        int32 Data = 0;

        inline static const FTypeInfo TypeInfo{ "UObjectTest", nullptr, nullptr };
        virtual const FTypeInfo* GetTypeInfo() const noexcept override { return &TypeInfo; }

    protected:
        void Serialize(FArchive& Archive) override
        {
            UObject::Serialize(Archive);
            Archive.Serialize("Data", Data);
        }
    };

    TEST_CASE("Undo / Redo Flow with FMessageChannel")
    {
        // 1. 초기 환경 세팅
        auto TestObject = std::make_unique<UObjectTest>();
        TestObject->Data = 3;
        UObjectSystem::Register(TestObject.get());

        // 2. 메시지 채널과 Undo 시스템 연결
        FMessageChannel WorldChannel(100);
        FUndoSystem::InitializeSenderToWorldChannel(WorldChannel.GetSender());

        // 메시지 처리용 람다 바인드
        WorldChannel.TryBind<FMessageUndoObjectStateChanged>([](const FMessageUndoObjectStateChanged& Payload)
            {
                UObject* Target = UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(Payload.TargetGuid));
                if (Target)
                {
                    // 리시버가 진짜로 데이터를 덮어씌움
                    FArchiveMemory ArchiveLoad(Payload.SavedData);
                    Target->Load(ArchiveLoad);
                }
            });

        // =================================================================
        // [Transaction 1] Data: 3 -> 5
        // =================================================================
        FUndoSystem::BeginTransaction("Change Data to 5");
        FUndoSystem::RecordObject(TestObject.get(), EUndoType::StateChange, nullptr);
        TestObject->Data = 5;
        FUndoSystem::EndTransaction();

        // =================================================================
        // [Transaction 2] Data: 5 -> 10
        // =================================================================
        FUndoSystem::BeginTransaction("Change Data to 10");
        FUndoSystem::RecordObject(TestObject.get(), EUndoType::StateChange, nullptr);
        TestObject->Data = 10;
        FUndoSystem::EndTransaction();

        CHECK_EQ(TestObject->Data, 10);

        // =================================================================
        // [UNDO 검증 1] 10 -> 5
        // =================================================================
        FUndoSystem::Undo();         // 1. Undo 실행: 채널에 FMessageUndoObjectStateChanged 푸쉬됨
        WorldChannel.Dispatch();     // 2. 채널 디스패치: 방금 바인딩한 람다가 실행되며 덮어쓰기 완료

        CHECK_EQ(TestObject->Data, 5);

        // =================================================================
        // [UNDO 검증 2] 5 -> 3
        // =================================================================
        FUndoSystem::Undo();
        WorldChannel.Dispatch();

        CHECK_EQ(TestObject->Data, 3);

        // =================================================================
        // [REDO 검증] 3 -> 5
        // =================================================================
        FUndoSystem::Redo();
        WorldChannel.Dispatch();

        CHECK_EQ(TestObject->Data, 5);

        // =================================================================
        // [History Truncation 검증] 과거로 돌아간 상태에서 대체 역사 생성
        // =================================================================
        FUndoSystem::BeginTransaction("Alternative Timeline: Change to 7");
        FUndoSystem::RecordObject(TestObject.get(), EUndoType::StateChange, nullptr);
        TestObject->Data = 7;
        FUndoSystem::EndTransaction();

        CHECK_EQ(TestObject->Data, 7);

        FUndoSystem::Redo();
        WorldChannel.Dispatch(); // 10으로 가는 역사가 잘렸으므로 핸들러가 실행되어도 값은 변함없어야 함

        CHECK_EQ(TestObject->Data, 7);
    }
}
