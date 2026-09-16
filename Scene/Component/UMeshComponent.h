#pragma once

#include "UPrimitiveComponent.h"
#include "Core/Asset/FAssetHandle.h"

class UMesh;

class UMeshComponent : public UPrimitiveComponent {
public:
    UMeshComponent() = default;
    ~UMeshComponent() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UMeshComponent, UPrimitiveComponent)

    FAssetHandle GetMeshHandle() const;
    void SetMeshHandle(FAssetHandle InHandle);
    void DrawPanels(FPropertyEditorContext& Context) override;
    void OnRegister() override;
    
    virtual UMesh* ResolveMesh() const;
    bool BuildPickingBoxFromMesh();
    bool RaycastMesh(const FRay& Ray, float& OutDistance) const;

protected:
    void Serialize(FArchive& Archive) override;
private:
    FAssetHandle MeshHandle;
};
