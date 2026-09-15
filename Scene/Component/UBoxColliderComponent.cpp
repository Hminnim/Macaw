#include "PCH.h"
#include "UBoxColliderComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

#include "UMeshComponent.h"
#include "Scene/AActor.h"
#include "Core/Asset/UMesh.h"
#include "Core/Base/UObjectSystem.h"

void UBoxColliderComponent::SetMeshComponent(UMeshComponent* InMeshComponent) {
    MeshComponent.Set(InMeshComponent);
    PendingMeshComponentGuid = {};
    BuildBoundsFromMesh();
}

UMeshComponent* UBoxColliderComponent::GetMeshComponent() const {
    return MeshComponent.Get();
}

bool UBoxColliderComponent::BuildBoundsFromMesh() {
    UMeshComponent* Mesh = MeshComponent.Get();
    UMesh* Asset = Mesh != nullptr ? Mesh->ResolveMesh() : nullptr;
    if (Asset == nullptr) {
        return false;
    }

    const auto Positions = Asset->GetVertexAttributeData<EVertexAttribute::Position>();
    if (Positions.empty()) {
        return false;
    }

    std::vector<DirectX::XMFLOAT3> Points;
    Points.reserve(Positions.size());
    for (const FVector3& Position : Positions) {
        Points.emplace_back(Position.x, Position.y, Position.z);
    }

    DirectX::BoundingBox Bounds;
    DirectX::BoundingBox::CreateFromPoints(Bounds, Points.size(), Points.data(), sizeof(DirectX::XMFLOAT3));
    DirectX::BoundingOrientedBox::CreateFromBoundingBox(OBB, Bounds);
    SetPickingBox(OBB);
    return true;
}

bool UBoxColliderComponent::RaycastBounds(const FRay& Ray, float& OutDistance) const {
    DirectX::BoundingOrientedBox WorldBox;
    OBB.Transform(WorldBox, GetComponentToWorld().ToSimpleMath());
    return WorldBox.Intersects(Ray.position, Ray.direction, OutDistance);
}

FVector3 UBoxColliderComponent::GetBoundsCenter() const {
    return FVector3{ OBB.Center.x, OBB.Center.y, OBB.Center.z };
}

FVector3 UBoxColliderComponent::GetExtent() const {
    return FVector3{ OBB.Extents.x, OBB.Extents.y, OBB.Extents.z };
}

FQuat UBoxColliderComponent::GetBoundsOrientation() const {
    return FQuat{ OBB.Orientation.x, OBB.Orientation.y, OBB.Orientation.z, OBB.Orientation.w };
}

void UBoxColliderComponent::SetExtent(const FVector3& InExtent) {
    OBB.Extents = DirectX::XMFLOAT3(InExtent.x, InExtent.y, InExtent.z);
    SetPickingBox(OBB);
}

void UBoxColliderComponent::DrawPanels(FPropertyEditorContext& Context) {
    UCollisionComponent::DrawPanels(Context);
    Context.DrawVector3("Extent", GetExtent(), 0.05f, 0.001f, FLT_MAX, [this](const FVector3& Extent) {
        SetExtent(Extent);
    });

    AActor* Actor = GetOwner();
    if (Actor == nullptr) {
        return;
    }
    UMeshComponent* CurrentMesh = GetMeshComponent();
    const char* Preview = CurrentMesh != nullptr ? CurrentMesh->GetTypeInfo()->TypeName.data() : "None";
    std::vector<FPropertyReferenceOption> Candidates;
    for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
        auto* Mesh = dynamic_cast<UMeshComponent*>(Candidate.get());
        if (Mesh == nullptr) {
            continue;
        }
        Candidates.push_back({ Mesh, FString(Mesh->GetTypeInfo()->TypeName), Mesh == CurrentMesh, [this, Mesh] {
            SetMeshComponent(Mesh);
        } });
    }
    Context.DrawReferencePicker("Source Mesh Component", Preview, CurrentMesh == nullptr, [this] {
        SetMeshComponent(nullptr);
    }, Candidates);
    Context.DrawButton("Build Bounds From Mesh", [this] {
        BuildBoundsFromMesh();
    });
}

bool UBoxColliderComponent::ResolveLoadedReferences() {
    if (!UCollisionComponent::ResolveLoadedReferences()) {
        return false;
    }
    if (PendingMeshComponentGuid.IsValid()) {
        UObject* Object = UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(PendingMeshComponentGuid));
        if (Object == nullptr || !Object->GetTypeInfo()->IsA(UMeshComponent::StaticTypeInfo())) {
            return false;
        }
        MeshComponent.Set(static_cast<UMeshComponent*>(Object));
        PendingMeshComponentGuid = {};
    }
    BuildBoundsFromMesh();
    return true;
}

void UBoxColliderComponent::InitializeComponent() {
    UCollisionComponent::InitializeComponent();
    BuildBoundsFromMesh();
}

void UBoxColliderComponent::Serialize(FArchive& Archive) {
    UCollisionComponent::Serialize(Archive);

    FString MeshComponentGuid;
    if (UMeshComponent* Mesh = MeshComponent.Get()) {
        MeshComponentGuid = Mesh->GetGuid().ToString();
    }
    Archive.Serialize("GuidMeshComponent", MeshComponentGuid);
    if (Archive.IsLoading() && !MeshComponentGuid.empty() && !PendingMeshComponentGuid.Parse(MeshComponentGuid)) {
        PendingMeshComponentGuid = {};
    }

    FVector3 Center(OBB.Center);
    FVector3 Extent(OBB.Extents);
    FQuat Orientation(OBB.Orientation);
    Archive.Serialize("OBB_Center", Center);
    Archive.Serialize("OBB_Extent", Extent);
    Archive.Serialize("OBB_Orientation", Orientation);
    if (Archive.IsLoading()) {
        OBB.Center = Center.ToSimpleMath();
        OBB.Extents = Extent.ToSimpleMath();
        OBB.Orientation = Orientation.ToSimpleMath();
        SetPickingBox(OBB);
    }
}
