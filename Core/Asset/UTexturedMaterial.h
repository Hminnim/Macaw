#pragma once

#include "UMaterial.h"
#include "../Base/TypeInfo.h"

class UTexturedMaterial : public UMaterial {
public:
    UTexturedMaterial() = default;
    ~UTexturedMaterial() override = default;

    UTexturedMaterial(const UTexturedMaterial&) = delete;
    UTexturedMaterial& operator=(const UTexturedMaterial&) = delete;

    UTexturedMaterial(UTexturedMaterial&&) noexcept = default;
    UTexturedMaterial& operator=(UTexturedMaterial&&) noexcept = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UTexturedMaterial, UMaterial);

    virtual void Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) override;

    virtual FMaterialChunkSignature BuildChunkSignature() const override;
    virtual void Finalize(IAssetQuery* Query) override;
    virtual void Serialize(FArchive& Ar) override;

private:
	FAssetHandle TextureHandle{};
    FString TextureName{};
};
