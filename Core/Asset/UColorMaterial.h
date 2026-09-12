#pragma once

#include "UMaterial.h"
#include "../Base/TypeInfo.h"

struct FColorMaterialGPUData {
    FVector4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    FVector4 Reserved0{};
    FVector4 Reserved1{};
    FVector4 Reserved2{};
    FVector4 Reserved3{};
    FVector4 Reserved4{};
    FVector4 Reserved5{};
    FVector4 Reserved6{};
};

static_assert(sizeof(FColorMaterialGPUData) == MATERIAL_GPU_STRIDE);

class UColorMaterial : public UMaterial {
public:
    UColorMaterial() = default;
    ~UColorMaterial() override = default;

    UColorMaterial(const UColorMaterial&) = delete;
    UColorMaterial& operator=(const UColorMaterial&) = delete;

    UColorMaterial(UColorMaterial&&) noexcept = default;
    UColorMaterial& operator=(UColorMaterial&&) noexcept = default;

public:
	JG_DECLARE_DERIVED_TYPEINFO(UColorMaterial, UMaterial);

    virtual void Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) override;

    virtual void BuildGPUData(FMaterialGPUSlot& OutSlot) const override;
	virtual void Finalize(IAssetQuery* Query) override;
	virtual void Serialize(FArchive& Ar) override;
public:
    void SetColor(const FVector4& InColor);
    const FVector4& GetColor() const { return Color; }

private:
    FColor4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
};