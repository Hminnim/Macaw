#pragma once

#include "FTextureProfile.h"
#include "UMaterial.h"
#include "../Base/TypeInfo.h"

struct FTexturedMaterialGPUData {
    FPackedTextureLocation Location;
    FVector3 Padding{};
    FVector4 Reserved0{};
    FVector4 Reserved1{};
    FVector4 Reserved2{};
    FVector4 Reserved3{};
    FVector4 Reserved4{};
    FVector4 Reserved5{};
    FVector4 Reserved6{};
};

static_assert(sizeof(FTexturedMaterialGPUData) == MATERIAL_GPU_STRIDE);

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

    virtual void BuildGPUData(FMaterialGPUSlot& OutSlot) const override;
    virtual void Finalize(IAssetQuery* Query) override;
    virtual void Serialize(FArchive& Ar) override;

private:
	FTextureLocation Location{};
    FString TextureName{}; 
};