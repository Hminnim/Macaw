#include "PCH.h"
#include "doctest.h"

#include "../Render/RenderWindowInfo.h"
#include "../Scene/AActor.h"
#include "../Scene/Component/USceneComponent.h"
#include "../Scene/UWorld.h"

namespace
{
    enum class EActorLifecycleEvent
    {
        Added,
        Initialized,
        BegunPlay,
        Tick,
        EndedPlay,
        Removed
    };

    TArray<EActorLifecycleEvent> GActorLifecycleEvents;

    struct FComponentLifecycleCounters
    {
        int InitializeCalls = 0;
        int BeginPlayCalls = 0;
        int EndPlayCalls = 0;
    };

    FComponentLifecycleCounters GComponentLifecycleCounters;

    class ULifecycleTrackingComponent final : public UActorComponent
    {
    public:
        void InitializeComponent() override
        {
            UActorComponent::InitializeComponent();
            ++GComponentLifecycleCounters.InitializeCalls;
        }

        void BeginPlay() override
        {
            UActorComponent::BeginPlay();
            ++GComponentLifecycleCounters.BeginPlayCalls;
        }

        void EndPlay() override
        {
            UActorComponent::EndPlay();
            ++GComponentLifecycleCounters.EndPlayCalls;
        }
    };

    class ALifecycleTrackingActor final : public AActor
    {
    public:
        void Tick(float DeltaTime) override
        {
            GActorLifecycleEvents.push_back(EActorLifecycleEvent::Tick);
            AActor::Tick(DeltaTime);
        }

    protected:
        void OnAddedToWorld() override
        {
            GActorLifecycleEvents.push_back(EActorLifecycleEvent::Added);
        }

        void InitializeComponents() override
        {
            GActorLifecycleEvents.push_back(EActorLifecycleEvent::Initialized);
        }

        void BeginPlay() override
        {
            GActorLifecycleEvents.push_back(EActorLifecycleEvent::BegunPlay);
        }

        void EndPlay() override
        {
            GActorLifecycleEvents.push_back(EActorLifecycleEvent::EndedPlay);
        }

        void OnRemovedFromWorld() override
        {
            GActorLifecycleEvents.push_back(EActorLifecycleEvent::Removed);
        }
    };
}

TEST_SUITE("CH3 Actor Lifecycle")
{
    TEST_CASE("Actor enters, ticks, and leaves a world in lifecycle order")
    {
        GActorLifecycleEvents.clear();

        UWorld World;
        FStateChannel<RenderWindowInfo> WindowInfoChannel;
        World.SetWindowInfoReader(WindowInfoChannel.GetReader());

        ALifecycleTrackingActor* Actor = World.AdoptActor<ALifecycleTrackingActor>();
        REQUIRE(Actor != nullptr);

        CHECK(Actor->HasBegunPlay());
        REQUIRE_EQ(GActorLifecycleEvents.size(), 3);
        CHECK_EQ(GActorLifecycleEvents[0], EActorLifecycleEvent::Added);
        CHECK_EQ(GActorLifecycleEvents[1], EActorLifecycleEvent::Initialized);
        CHECK_EQ(GActorLifecycleEvents[2], EActorLifecycleEvent::BegunPlay);

        World.Tick(0.016f);
        REQUIRE_EQ(GActorLifecycleEvents.size(), 4);
        CHECK_EQ(GActorLifecycleEvents[3], EActorLifecycleEvent::Tick);

        REQUIRE(World.DestroyActor(Actor));
        World.FlushPendingDestroyActors();

        REQUIRE_EQ(GActorLifecycleEvents.size(), 6);
        CHECK_EQ(GActorLifecycleEvents[4], EActorLifecycleEvent::EndedPlay);
        CHECK_EQ(GActorLifecycleEvents[5], EActorLifecycleEvent::Removed);
    }

    TEST_CASE("Actor transform APIs delegate to an owned root component")
    {
        UWorld World;
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        CHECK_FALSE(Actor->SetActorLocation({ 1.0f, 2.0f, 3.0f }));

        USceneComponent* Root = Actor->AddComponent<USceneComponent>();
        REQUIRE(Root != nullptr);
        REQUIRE(Actor->SetRootComponent(Root));

        REQUIRE(Actor->SetActorLocation({ 1.0f, 2.0f, 3.0f }));
        CHECK_EQ(Actor->GetActorLocation(), FVector3(1.0f, 2.0f, 3.0f));
        CHECK_EQ(Root->GetRelativeLocation(), FVector3(1.0f, 2.0f, 3.0f));
    }

    TEST_CASE("Component lifecycle follows actor entry, dynamic add, and removal")
    {
        GComponentLifecycleCounters = {};

        UWorld World;
        auto PendingActor = std::make_unique<AActor>();
        ULifecycleTrackingComponent* InitialComponent =
            PendingActor->AddComponent<ULifecycleTrackingComponent>();
        REQUIRE(InitialComponent != nullptr);

        CHECK_FALSE(InitialComponent->IsInitialized());
        CHECK_FALSE(InitialComponent->HasBegunPlay());

        AActor* Actor = World.AddActor(std::move(PendingActor));
        REQUIRE(Actor != nullptr);
        CHECK(InitialComponent->IsInitialized());
        CHECK(InitialComponent->HasBegunPlay());
        CHECK_EQ(GComponentLifecycleCounters.InitializeCalls, 1);
        CHECK_EQ(GComponentLifecycleCounters.BeginPlayCalls, 1);

        ULifecycleTrackingComponent* DynamicComponent =
            Actor->AddComponent<ULifecycleTrackingComponent>();
        REQUIRE(DynamicComponent != nullptr);
        CHECK(DynamicComponent->IsInitialized());
        CHECK(DynamicComponent->HasBegunPlay());
        CHECK_EQ(GComponentLifecycleCounters.InitializeCalls, 2);
        CHECK_EQ(GComponentLifecycleCounters.BeginPlayCalls, 2);

        REQUIRE(Actor->DestroyComponent(DynamicComponent));
        CHECK_EQ(GComponentLifecycleCounters.EndPlayCalls, 1);

        REQUIRE(World.DestroyActor(Actor));
        World.FlushPendingDestroyActors();
        CHECK_EQ(GComponentLifecycleCounters.EndPlayCalls, 2);
    }

    TEST_CASE("Actor rejects a root component owned by another actor")
    {
        UWorld World;
        AActor* FirstActor = World.AdoptActor<AActor>();
        AActor* SecondActor = World.AdoptActor<AActor>();
        REQUIRE(FirstActor != nullptr);
        REQUIRE(SecondActor != nullptr);

        USceneComponent* ForeignRoot = SecondActor->AddComponent<USceneComponent>();
        REQUIRE(ForeignRoot != nullptr);

        CHECK_FALSE(FirstActor->SetRootComponent(ForeignRoot));
        CHECK_EQ(FirstActor->GetRootComponent(), nullptr);
    }
}
