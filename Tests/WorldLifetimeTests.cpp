#include "PCH.h"
#include "doctest.h"

#include "../Core/Base/UObjectSystem.h"
#include "../Render/RenderWindowInfo.h"
#include "../Scene/AActor.h"
#include "../Scene/Component/UActorComponent.h"
#include "../Scene/Component/USceneComponent.h"
#include "../Scene/UWorld.h"

namespace
{
    struct FLifetimeCounters
    {
        int ActorDestructions = 0;
        int ComponentCreations = 0;
        int ComponentDestructions = 0;
        int ComponentDestructorCalls = 0;
        bool OwnerWasValidOnCreate = false;
        bool OwnerWasValidOnDestroy = false;
    };

    FLifetimeCounters GLifetimeCounters;

    void ResetLifetimeCounters()
    {
        GLifetimeCounters = {};
    }

    class ALifetimeActor final : public AActor
    {
    public:
        ~ALifetimeActor() override
        {
            ++GLifetimeCounters.ActorDestructions;
        }
    };

    class ULifetimeComponent final : public UActorComponent
    {
    public:
        ~ULifetimeComponent() override
        {
            ++GLifetimeCounters.ComponentDestructorCalls;
        }

        void OnRegister() override
        {
            ++GLifetimeCounters.ComponentCreations;
            GLifetimeCounters.OwnerWasValidOnCreate =
                GetOwner() != nullptr && GetOwner()->GetWorld() != nullptr;
        }

        void OnUnregister() override
        {
            ++GLifetimeCounters.ComponentDestructions;
            GLifetimeCounters.OwnerWasValidOnDestroy = GetOwner() != nullptr;

            UActorComponent::OnUnregister();
        }
    };
}

TEST_SUITE("World Lifetime")
{
    TEST_CASE("World adopts a preconfigured actor and destroys its component subtree")
    {
        ResetLifetimeCounters();
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        FObjectHandle ActorHandle;
        FObjectHandle ComponentHandle;

        {
            UWorld World;

            auto NewActor = std::make_unique<ALifetimeActor>();
            ULifetimeComponent* Component = NewActor->AddComponent<ULifetimeComponent>();

            CHECK_EQ(GLifetimeCounters.ComponentCreations, 0);
            CHECK_EQ(Component->GetOwner(), NewActor.get());

            ALifetimeActor* Actor = static_cast<ALifetimeActor*>(World.AddActor(std::move(NewActor)));

            REQUIRE(Actor != nullptr);
            ActorHandle = Actor->GetHandle();
            ComponentHandle = Component->GetHandle();

            CHECK_EQ(World.GetActors().size(), 1);
            CHECK_EQ(Actor->GetWorld(), &World);
            CHECK_EQ(GLifetimeCounters.ComponentCreations, 1);
            CHECK(GLifetimeCounters.OwnerWasValidOnCreate);
            CHECK_EQ(UObjectSystem::Resolve(ActorHandle), Actor);
            CHECK_EQ(UObjectSystem::Resolve(ComponentHandle), Component);
        }

        CHECK_EQ(GLifetimeCounters.ActorDestructions, 1);
        CHECK_EQ(GLifetimeCounters.ComponentDestructions, 1);
        CHECK_EQ(GLifetimeCounters.ComponentDestructorCalls, 1);
        CHECK(GLifetimeCounters.OwnerWasValidOnDestroy);
        CHECK_EQ(UObjectSystem::Resolve(ActorHandle), nullptr);
        CHECK_EQ(UObjectSystem::Resolve(ComponentHandle), nullptr);
        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }

    TEST_CASE("DestroyComponent only releases the selected owned component")
    {
        ResetLifetimeCounters();
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        {
            UWorld World;
            ALifetimeActor* Actor = World.AdoptActor<ALifetimeActor>();
            REQUIRE(Actor != nullptr);

            ULifetimeComponent* Component = Actor->AddComponent<ULifetimeComponent>();
            const FObjectHandle ComponentHandle = Component->GetHandle();

            Component->DestroyComponent();

            CHECK(Actor->GetComponents().empty());
            CHECK_EQ(GLifetimeCounters.ComponentCreations, 1);
            CHECK_EQ(GLifetimeCounters.ComponentDestructions, 1);
            CHECK_EQ(GLifetimeCounters.ComponentDestructorCalls, 1);
            CHECK(GLifetimeCounters.OwnerWasValidOnDestroy);
            CHECK_EQ(UObjectSystem::Resolve(ComponentHandle), nullptr);
            CHECK_EQ(World.GetActors().size(), 1);
            CHECK_EQ(GLifetimeCounters.ActorDestructions, 0);
        }

        CHECK_EQ(GLifetimeCounters.ActorDestructions, 1);
        CHECK_EQ(GLifetimeCounters.ComponentDestructions, 1);
        CHECK_EQ(GLifetimeCounters.ComponentDestructorCalls, 1);
        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }

    TEST_CASE("Destroying a scene parent detaches its children and clears the root")
    {
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        {
            UWorld World;
            AActor* Actor = World.AdoptActor<AActor>();
            REQUIRE(Actor != nullptr);

            USceneComponent* Parent = Actor->AddComponent<USceneComponent>();
            USceneComponent* Child = Actor->AddComponent<USceneComponent>();
            Actor->SetRootComponent(Parent);
            Child->AttachToComponent(Parent);

            const FObjectHandle ParentHandle = Parent->GetHandle();

            REQUIRE_EQ(Child->GetParent(), Parent);
            REQUIRE_EQ(Parent->GetChildren().size(), 1);
            Parent->DestroyComponent();

            CHECK_EQ(Actor->GetRootComponent(), nullptr);
            CHECK_EQ(Child->GetParent(), nullptr);
            CHECK(Child->GetChildren().empty());
            CHECK_EQ(UObjectSystem::Resolve(ParentHandle), nullptr);
            CHECK_EQ(Actor->GetComponents().size(), 1);
        }

        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }

    TEST_CASE("DestroyComponent promotes scene children when requested")
    {
        UWorld World;
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        USceneComponent* Root = Actor->AddComponent<USceneComponent>();
        USceneComponent* Parent = Actor->AddComponent<USceneComponent>();
        USceneComponent* Child = Actor->AddComponent<USceneComponent>();
        REQUIRE(Actor->SetRootComponent(Root));
        REQUIRE(Parent->AttachToComponent(Root));
        REQUIRE(Child->AttachToComponent(Parent));

        Root->SetRelativeLocation({ 10.0f, 0.0f, 0.0f });
        Parent->SetRelativeLocation({ 5.0f, 0.0f, 0.0f });
        Child->SetRelativeLocation({ 2.0f, 0.0f, 0.0f });
        const FVector3 ChildWorldLocation = Child->GetComponentLocation();

        Parent->DestroyComponent(true);

        CHECK_EQ(Actor->GetRootComponent(), Root);
        CHECK_EQ(Child->GetParent(), Root);
        CHECK_EQ(Child->GetComponentLocation(), ChildWorldLocation);
        CHECK_EQ(Actor->GetComponents().size(), 2);
    }

    TEST_CASE("Pending actor destruction is flushed at the end of a world tick")
    {
        ResetLifetimeCounters();
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        {
            UWorld World;
            FStateChannel<RenderWindowInfo> WindowInfoChannel;
            World.SetWindowInfoReader(WindowInfoChannel.GetReader());

            ALifetimeActor* Actor = World.AdoptActor<ALifetimeActor>();
            REQUIRE(Actor != nullptr);
            ULifetimeComponent* Component = Actor->AddComponent<ULifetimeComponent>();

            const FObjectHandle ActorHandle = Actor->GetHandle();
            const FObjectHandle ComponentHandle = Component->GetHandle();

            REQUIRE(World.DestroyActor(Actor));
            CHECK_EQ(World.GetActors().size(), 1);
            CHECK_EQ(UObjectSystem::Resolve(ActorHandle), Actor);

            World.Tick(0.0f);

            CHECK(World.GetActors().empty());
            CHECK_EQ(GLifetimeCounters.ActorDestructions, 1);
            CHECK_EQ(GLifetimeCounters.ComponentDestructions, 1);
            CHECK_EQ(GLifetimeCounters.ComponentDestructorCalls, 1);
            CHECK_EQ(UObjectSystem::Resolve(ActorHandle), nullptr);
            CHECK_EQ(UObjectSystem::Resolve(ComponentHandle), nullptr);
        }

        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }
}
