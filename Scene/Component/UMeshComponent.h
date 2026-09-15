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
    virtual UMesh* ResolveMesh() const;
    bool RaycastMesh(const FRay& Ray, float& OutDistance) const;

protected:
    void Serialize(FArchive& Archive) override;

private:
    FAssetHandle MeshHandle;
};
