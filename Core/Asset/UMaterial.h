#pragma once

#include "../Base/UObject.h"
#include "FMaterialChunkSignature.h"
#include "FMaterialGPUData.h"

#include "UAsset.h"
#include "Common.h"

#include <d3d11.h>

class FMaterialBuffer;

class UMaterial : public UAsset {
public:
    UMaterial() = default;
    virtual ~UMaterial() = default;

    UMaterial(const UMaterial&) = delete;
    UMaterial& operator=(const UMaterial&) = delete;

    UMaterial(UMaterial&&) noexcept = default;
    UMaterial& operator=(UMaterial&&) noexcept = default;

public:
	JG_DECLARE_DERIVED_TYPEINFO(UMaterial, UAsset);

    virtual void Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) override;
    virtual void BuildGPUData(FMaterialGPUSlot& OutSlot) const;
    virtual FMaterialChunkSignature BuildChunkSignature() const;
    virtual void Finalize(IAssetQuery* Query);

    uint32 GetGPUIndex() const { return GPUIndex; }

    void MarkGPUDataDirty() { bGPUDataDirty = true; }

protected:
	virtual void Serialize(FArchive& Ar) override;

private:
    friend class FMaterialBuffer;

    uint32 GPUIndex{ UINT32_MAX };
    bool bGPUDataDirty{ true };
};
