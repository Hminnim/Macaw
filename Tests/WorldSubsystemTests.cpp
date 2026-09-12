#include "PCH.h"
#include "doctest.h"

#include "../Scene/AActor.h"
#include "../Scene/Component/UCameraComponent.h"
#include "../Scene/Component/UBoxColliderComponent.h"
#include "../Scene/Component/UMeshComponent.h"
#include "../Scene/Component/UStaticMeshComponent.h"
#include "../Scene/FWorldEditorContext.h"
#include "../Scene/Subsystem/UCameraSubsystem.h"
#include "../Scene/Subsystem/UCollisionSubsystem.h"
#include "../Scene/Subsystem/URenderSubsystem.h"
#include "../Scene/UWorld.h"

#include "../Core/Asset/UMesh.h"
#include "../Core/Asset/BasicGeometry/Plane.h"

namespace {
    template<typename T>
    concept CH8MeshHasBounds = requires(const T& Mesh) {
        Mesh.GetLocalBoundingBox();
    };

    static_assert(!CH8MeshHasBounds<UMesh>);

    class UTestMeshComponent final : public UMeshComponent {
    public:
        JG_DECLARE_DERIVED_TYPEINFO(UTestMeshComponent, UMeshComponent)

        UMesh* ResolveMesh() const override {
            return Mesh;
        }

        UMesh* Mesh = nullptr;
    };

    Microsoft::WRL::ComPtr<ID3D11Device> CreateTestDevice() {
        Microsoft::WRL::ComPtr<ID3D11Device> Device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
        D3D_FEATURE_LEVEL FeatureLevel{};
        const HRESULT Result = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &Device,
            &FeatureLevel,
            &Context
        );
        CHECK(SUCCEEDED(Result));
        return Device;
    }

    bool MakeTriangleMesh(UMesh& Mesh, ID3D11Device* Device) {
        const std::array<uint32, 3> Indices{ 0, 1, 2 };
        const std::array<FVector3, 3> Positions{
            FVector3{ -1.0f, 0.0f, -1.0f },
            FVector3{ 1.0f, 0.0f, -1.0f },
            FVector3{ -1.0f, 0.0f, 1.0f }
        };
        return Mesh.Make(Device, Indices, MakeVertexAttribute<EVertexAttribute::Position>(Positions));
    }
}

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
        UBoxColliderComponent* Collision = Actor->AddComponent<UBoxColliderComponent>();
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

    TEST_CASE("Editor context owns selection state and selected render flags") {
        UWorld World;
        FWorldEditorContext Context;
        World.SetEditorContext(&Context);

        AActor* SelectedActor = World.AdoptActor<AActor>();
        AActor* OtherActor = World.AdoptActor<AActor>();
        REQUIRE(SelectedActor != nullptr);
        REQUIRE(OtherActor != nullptr);

        UStaticMeshComponent* SelectedMesh = SelectedActor->AddComponent<UStaticMeshComponent>();
        UStaticMeshComponent* OtherMesh = OtherActor->AddComponent<UStaticMeshComponent>();
        UBoxColliderComponent* Collider = SelectedActor->AddComponent<UBoxColliderComponent>();
        REQUIRE(SelectedMesh != nullptr);
        REQUIRE(OtherMesh != nullptr);
        REQUIRE(Collider != nullptr);
        SelectedActor->SetRootComponent(SelectedMesh);
        OtherActor->SetRootComponent(OtherMesh);

        Context.SetSelectedCollider(Collider, 11);
        auto Selection = Context.GetSelectionStateReader();
        REQUIRE(Selection.HasValue());
        CHECK_EQ(Context.GetSelectedActor(), SelectedActor);
        CHECK_EQ(Selection.Read().PickedColliderHandle, Collider->GetHandle());
        CHECK_EQ(Selection.Read().TransformRevision, 11);

        FRenderProbe Probe;
        World.GetRenderSubsystem().BuildRenderProbes(Probe);
        REQUIRE_EQ(Probe.ActorProbes.size(), 2);
        CHECK((Probe.ActorProbes[0].Flags & static_cast<uint32>(ERenderObjectFlags::Selected)) != 0);
        CHECK((Probe.ActorProbes[1].Flags & static_cast<uint32>(ERenderObjectFlags::Selected)) == 0);

        REQUIRE(World.DestroyActor(SelectedActor));
        World.FlushPendingDestroyActors();
        CHECK_FALSE(Context.GetSelectionStateReader().HasValue());
    }

    TEST_CASE("Box colliders build mesh bounds and require connected mesh narrow phase") {
        Microsoft::WRL::ComPtr<ID3D11Device> Device = CreateTestDevice();
        REQUIRE(Device != nullptr);

        UMesh Mesh;
        REQUIRE(MakeTriangleMesh(Mesh, Device.Get()));

        UWorld World;
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);
        UTestMeshComponent* MeshComponent = Actor->AddComponent<UTestMeshComponent>();
        UBoxColliderComponent* Collider = Actor->AddComponent<UBoxColliderComponent>();
        REQUIRE(MeshComponent != nullptr);
        REQUIRE(Collider != nullptr);
        Actor->SetRootComponent(MeshComponent);
        MeshComponent->Mesh = &Mesh;
        Collider->SetMeshComponent(MeshComponent);

        CHECK_EQ(Collider->GetMeshComponent(), MeshComponent);
        CHECK(Collider->BuildBoundsFromMesh());
        CHECK(Collider->GetExtent().x == doctest::Approx(1.0f));
        CHECK(Collider->GetExtent().z == doctest::Approx(1.0f));
        float MissDistance = 0.0f;
        CHECK_FALSE(Collider->Raycast(FRay{ FVector3{ 0.75f, 0.75f, -1.0f }.ToSimpleMath(), FVector3{ 0.0f, 0.0f, 1.0f }.ToSimpleMath() }, MissDistance));

        float Distance = 0.0f;
        CHECK(MeshComponent->RaycastMesh(FRay{ FVector3{ -0.5f, -0.5f, -1.0f }.ToSimpleMath(), FVector3{ 0.0f, 0.0f, 1.0f }.ToSimpleMath() }, Distance));

        UBoxColliderComponent* BoundsOnly = Actor->AddComponent<UBoxColliderComponent>();
        REQUIRE(BoundsOnly != nullptr);
        BoundsOnly->SetExtent(FVector3{ 1.0f, 1.0f, 1.0f });
        CHECK(BoundsOnly->Raycast(FRay{ FVector3{ 0.0f, 0.0f, -2.0f }.ToSimpleMath(), FVector3{ 0.0f, 0.0f, 1.0f }.ToSimpleMath() }, Distance));
        CHECK(Distance == doctest::Approx(1.0f));

        CHECK(World.GetCollisionSubsystem().ContainsComponent(Collider));
        REQUIRE(World.DestroyActor(Actor));
        World.FlushPendingDestroyActors();
        CHECK_FALSE(World.GetCollisionSubsystem().ContainsComponent(Collider));
    }
}
