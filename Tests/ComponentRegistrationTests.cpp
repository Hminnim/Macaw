#include "PCH.h"
#include "doctest.h"

#include "../Core/Base/UObjectSystem.h"
#include "../Render/RenderWindowInfo.h"
#include "../Scene/AActor.h"
#include "../Scene/Component/UActorComponent.h"
#include "../Scene/UWorld.h"

namespace
{
    struct FRegistrationCounters
    {
        int RegisterCalls = 0;
        int UnregisterCalls = 0;
        int DestructorCalls = 0;
        bool OwnerWasAvailableOnRegister = false;
        bool OwnerWasAvailableOnUnregister = false;
        bool WorldWasAvailableOnUnregister = false;
        bool HandleWasResolvableOnUnregister = false;
        UWorld* WorldSeenOnRegister = nullptr;
        UWorld* WorldSeenOnUnregister = nullptr;
        bool WasUnregisteredBeforeDestruction = false;
    };

    FRegistrationCounters GRegistrationCounters;

    void ResetRegistrationCounters()
    {
        GRegistrationCounters = {};
    }

    class URegistrationTrackingComponent final : public UActorComponent
    {
    public:
        ~URegistrationTrackingComponent() override
        {
            ++GRegistrationCounters.DestructorCalls;
            GRegistrationCounters.WasUnregisteredBeforeDestruction =
                !IsRegistered() && GetBelongingWorld() == nullptr;
        }

        void OnRegister() override
        {
            ++GRegistrationCounters.RegisterCalls;
            GRegistrationCounters.OwnerWasAvailableOnRegister = GetOwner() != nullptr;
            GRegistrationCounters.WorldSeenOnRegister = GetBelongingWorld();
        }

        void OnUnregister() override
        {
            ++GRegistrationCounters.UnregisterCalls;
            GRegistrationCounters.OwnerWasAvailableOnUnregister = GetOwner() != nullptr;
            GRegistrationCounters.WorldWasAvailableOnUnregister =
                GetBelongingWorld() != nullptr &&
                GetOwner() != nullptr &&
                GetOwner()->GetWorld() != nullptr;
            GRegistrationCounters.HandleWasResolvableOnUnregister =
                UObjectSystem::Resolve(GetHandle()) == this;
            GRegistrationCounters.WorldSeenOnUnregister = GetBelongingWorld();
        }
    };
}

TEST_SUITE("CH1 Component Registration")
{
    TEST_CASE("A component configured before world entry registers exactly once on entry")
    {
        ResetRegistrationCounters();
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        {
            UWorld World;
            auto PendingActor = std::make_unique<AActor>();
            URegistrationTrackingComponent* Component =
                PendingActor->AddComponent<URegistrationTrackingComponent>();

            CHECK_FALSE(Component->IsRegistered());
            CHECK_EQ(Component->GetBelongingWorld(), nullptr);
            CHECK_EQ(GRegistrationCounters.RegisterCalls, 0);

            AActor* Actor = World.AddActor(std::move(PendingActor));
            REQUIRE(Actor != nullptr);

            CHECK(Component->IsRegistered());
            CHECK_EQ(Component->GetBelongingWorld(), &World);
            CHECK_EQ(GRegistrationCounters.RegisterCalls, 1);
            CHECK(GRegistrationCounters.OwnerWasAvailableOnRegister);
            CHECK_EQ(GRegistrationCounters.WorldSeenOnRegister, &World);
        }

        CHECK_EQ(GRegistrationCounters.UnregisterCalls, 1);
        CHECK_EQ(GRegistrationCounters.DestructorCalls, 1);
        CHECK(GRegistrationCounters.OwnerWasAvailableOnUnregister);
        CHECK(GRegistrationCounters.WorldWasAvailableOnUnregister);
        CHECK(GRegistrationCounters.WasUnregisteredBeforeDestruction);
        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }

    TEST_CASE("A component added to an in-world actor registers immediately")
    {
        ResetRegistrationCounters();
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        {
            UWorld World;
            AActor* Actor = World.AdoptActor<AActor>();
            REQUIRE(Actor != nullptr);

            URegistrationTrackingComponent* Component =
                Actor->AddComponent<URegistrationTrackingComponent>();

            CHECK(Component->IsRegistered());
            CHECK_EQ(Component->GetBelongingWorld(), &World);
            CHECK_EQ(GRegistrationCounters.RegisterCalls, 1);
            CHECK_EQ(GRegistrationCounters.WorldSeenOnRegister, &World);
        }

        CHECK_EQ(GRegistrationCounters.UnregisterCalls, 1);
        CHECK_EQ(GRegistrationCounters.DestructorCalls, 1);
        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }

    TEST_CASE("DestroyComponent unregisters the component before invalidating its object handle")
    {
        ResetRegistrationCounters();
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        {
            UWorld World;
            AActor* Actor = World.AdoptActor<AActor>();
            REQUIRE(Actor != nullptr);

            URegistrationTrackingComponent* Component =
                Actor->AddComponent<URegistrationTrackingComponent>();
            const FObjectHandle ComponentHandle = Component->GetHandle();

            REQUIRE(Actor->DestroyComponent(Component));

            CHECK_EQ(GRegistrationCounters.UnregisterCalls, 1);
            CHECK(GRegistrationCounters.OwnerWasAvailableOnUnregister);
            CHECK(GRegistrationCounters.WorldWasAvailableOnUnregister);
            CHECK(GRegistrationCounters.HandleWasResolvableOnUnregister);
            CHECK_EQ(GRegistrationCounters.WorldSeenOnUnregister, &World);
            CHECK_EQ(GRegistrationCounters.DestructorCalls, 1);
            CHECK(GRegistrationCounters.WasUnregisteredBeforeDestruction);
            CHECK_EQ(UObjectSystem::Resolve(ComponentHandle), nullptr);
            CHECK(Actor->GetComponents().empty());
        }

        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }

    TEST_CASE("Pending actor destruction unregisters components during the world tick flush")
    {
        ResetRegistrationCounters();
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        {
            UWorld World;
            FStateChannel<RenderWindowInfo> WindowInfoChannel;
            World.SetWindowInfoReader(WindowInfoChannel.GetReader());

            AActor* Actor = World.AdoptActor<AActor>();
            REQUIRE(Actor != nullptr);

            URegistrationTrackingComponent* Component =
                Actor->AddComponent<URegistrationTrackingComponent>();
            const FObjectHandle ComponentHandle = Component->GetHandle();

            REQUIRE(World.DestroyActor(Actor));
            CHECK(Component->IsRegistered());

            World.Tick(0.0f);

            CHECK_EQ(GRegistrationCounters.UnregisterCalls, 1);
            CHECK_EQ(GRegistrationCounters.DestructorCalls, 1);
            CHECK(GRegistrationCounters.WasUnregisteredBeforeDestruction);
            CHECK_EQ(UObjectSystem::Resolve(ComponentHandle), nullptr);
            CHECK(World.GetActors().empty());
        }

        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }
}
