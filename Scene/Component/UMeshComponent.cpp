#include "PCH.h"
#include "UMeshComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UMesh.h"

FAssetHandle UMeshComponent::GetMeshHandle() const {
    return MeshHandle;
}

void UMeshComponent::SetMeshHandle(FAssetHandle InHandle) {
    MeshHandle = InHandle;
}

void UMeshComponent::DrawPanels(FPropertyEditorContext& Context) {
    UPrimitiveComponent::DrawPanels(Context);
    Context.DrawMeshComponentProperties(*this);
}

UMesh* UMeshComponent::ResolveMesh() const {
    AActor* Owner = GetOwner();
    UWorld* World = Owner != nullptr ? Owner->GetWorld() : nullptr;
    FAssetRegistry* Registry = World != nullptr ? World->GetAssetRegistry() : nullptr;
    return Registry != nullptr ? Registry->ResolveAsset<UMesh>(MeshHandle) : nullptr;
}

bool UMeshComponent::RaycastMesh(const FRay& Ray, float& OutDistance) const {
    UMesh* Mesh = ResolveMesh();
    if (Mesh == nullptr) {
        return false;
    }

    const auto Positions = Mesh->GetVertexAttributeData<EVertexAttribute::Position>();
    const TArray<uint32>& Indices = Mesh->GetIndices();
    if (Positions.empty() || Indices.size() < 3) {
        return false;
    }

    bool bHit = false;
    float ClosestDistance = std::numeric_limits<float>::max();
    const FMatrix WorldMatrix = GetComponentToWorld();
    for (size_t Index = 0; Index + 2 < Indices.size(); Index += 3) {
        const uint32 I0 = Indices[Index];
        const uint32 I1 = Indices[Index + 1];
        const uint32 I2 = Indices[Index + 2];
        if (I0 >= Positions.size() || I1 >= Positions.size() || I2 >= Positions.size()) {
            continue;
        }

        const DirectX::XMVECTOR V0 = DirectX::XMVector3TransformCoord(Positions[I0].ToSimpleMath(), WorldMatrix.ToSimpleMath());
        const DirectX::XMVECTOR V1 = DirectX::XMVector3TransformCoord(Positions[I1].ToSimpleMath(), WorldMatrix.ToSimpleMath());
        const DirectX::XMVECTOR V2 = DirectX::XMVector3TransformCoord(Positions[I2].ToSimpleMath(), WorldMatrix.ToSimpleMath());
        float Distance = 0.0f;
        if (DirectX::TriangleTests::Intersects(Ray.position, Ray.direction, V0, V1, V2, Distance) && Distance < ClosestDistance) {
            ClosestDistance = Distance;
            bHit = true;
        }
    }

    if (bHit) {
        OutDistance = ClosestDistance;
    }
    return bHit;
}

void UMeshComponent::Serialize(FArchive& Archive) {
    UPrimitiveComponent::Serialize(Archive);

    FString MeshGuid;
    if (MeshHandle.ID != std::numeric_limits<uint32>::max()) {
        if (UAsset* Asset = Archive.GetAssetRegistry()->ResolveAsset<UAsset>(MeshHandle)) {
            MeshGuid = Asset->GetGuid().ToString();
        }
    }
    Archive.Serialize("GuidMeshHandle", MeshGuid);
    if (Archive.IsLoading() && !MeshGuid.empty()) {
        FGuid Guid;
        if (Guid.Parse(MeshGuid)) {
            MeshHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
        }
    }
}
