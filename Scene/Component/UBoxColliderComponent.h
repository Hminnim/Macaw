#pragma once

#include "UCollisionComponent.h"

class UMeshComponent;

class UBoxColliderComponent final : public UCollisionComponent {
public:
    UBoxColliderComponent() = default;
    ~UBoxColliderComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UBoxColliderComponent, UCollisionComponent)

    void SetMeshComponent(UMeshComponent* InMeshComponent);
    UMeshComponent* GetMeshComponent() const override;
    bool BuildBoundsFromMesh();

    bool RaycastBounds(const FRay& Ray, float& OutDistance) const override;
    void DrawEditorBounds(ILineRenderer& LineRenderer, ELineDepthMode DepthMode) const override;
    FVector3 GetExtent() const;
    void SetExtent(const FVector3& InExtent);
    void DrawPanels(FPropertyEditorContext& Context) override;

    bool ResolveLoadedReferences() override;
    void InitializeComponent() override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    TObjectRef<UMeshComponent> MeshComponent;
    FGuid PendingMeshComponentGuid{};
    DirectX::BoundingOrientedBox OBB{ DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f }, DirectX::XMFLOAT3{ 1.0f, 1.0f, 1.0f }, DirectX::XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
};

