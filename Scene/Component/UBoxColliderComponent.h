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
    FVector3 GetBoundsCenter() const override;
    FVector3 GetExtent() const override;
    FQuat GetBoundsOrientation() const override;
    void SetExtent(const FVector3& InExtent) override;

    bool ResolveLoadedReferences() override;
    void InitializeComponent() override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    TObjectRef<UMeshComponent> MeshComponent;
    FGuid PendingMeshComponentGuid{};
    DirectX::BoundingOrientedBox OBB{};
};

