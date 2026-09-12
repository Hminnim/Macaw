#include "PCH.h"
#include "doctest.h"

#include "../Scene/AActor.h"
#include "../Scene/Component/UCameraComponent.h"
#include "../Scene/Component/UCollisionComponent.h"
#include "../Scene/Component/UStaticMeshComponent.h"
#include "../Scene/Subsystem/UCameraSubsystem.h"
#include "../Scene/Subsystem/UCollisionSubsystem.h"
#include "../Scene/Subsystem/URenderSubsystem.h"
#include "../Scene/UWorld.h"

TEST_SUITE("CH6 World Subsystems") {
    TEST_CASE("World-owned subsystems track component registration and actor removal") {
        UWorld World;
        CHECK(World.GetRenderSubsystem().IsInitialized());
        CHECK(World.GetCollisionSubsystem().IsInitialized());
        CHECK(World.GetCameraSubsystem().IsInitialized());
        CHECK_EQ(World.GetRenderSubsystem().GetWorld(), &World);
        CHECK_EQ(World.GetCollisionSubsystem().GetWorld(), &World);
        CHECK_EQ(World.GetCameraSubsystem().GetWorld(), &World);

        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        UStaticMeshComponent* Mesh = Actor->AddComponent<UStaticMeshComponent>();
        UCollisionComponent* Collision = Actor->AddComponent<UCollisionComponent>();
        UCameraComponent* Camera = Actor->AddComponent<UCameraComponent>();
        REQUIRE(Mesh != nullptr);
        REQUIRE(Collision != nullptr);
        REQUIRE(Camera != nullptr);

        CHECK(World.GetRenderSubsystem().ContainsComponent(Mesh));
        CHECK(World.GetCollisionSubsystem().ContainsComponent(Collision));
        CHECK_EQ(World.GetCameraSubsystem().GetMainCamera(), Camera);

        Actor->SetWorld(nullptr);

        CHECK_FALSE(World.GetRenderSubsystem().ContainsComponent(Mesh));
        CHECK_FALSE(World.GetCollisionSubsystem().ContainsComponent(Collision));
        CHECK_EQ(World.GetCameraSubsystem().GetMainCamera(), nullptr);
    }
}
