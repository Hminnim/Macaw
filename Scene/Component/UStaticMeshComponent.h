#pragma once

#include "UPrimitiveComponent.h"
#include "Core/Asset/FAssetHandle.h"
#include "Serialize/FArchive.h"

class UStaticMeshComponent : public UPrimitiveComponent
{
public:
    UStaticMeshComponent() = default;
    ~UStaticMeshComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UStaticMeshComponent, UPrimitiveComponent)


    FAssetHandle GetMeshHandle() const;
    FAssetHandle GetMaterialHandle() const;
    FAssetHandle GetPipelineHandle() const;

    void SetMeshHandle(FAssetHandle InHandle);
    void SetMaterialHandle(FAssetHandle InHandle);
    void SetPipelineHandle(FAssetHandle InHandle);

    void OnRegister() override;
    void OnUnregister() override;
    virtual void MakeRender(FActorProbe& OutProbe) const override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    FAssetHandle MeshHandle;
    FAssetHandle MaterialHandle;
    FAssetHandle PipelineHandle;
};
