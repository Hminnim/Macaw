#include "PCH.h"
#include "UMaterial.h"

void UMaterial::Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) {
	UAsset::Initialize(Device, metaData);
}

void UMaterial::BuildGPUData(FMaterialGPUSlot& OutSlot) const {
    OutSlot = {};
}

FMaterialChunkSignature UMaterial::BuildChunkSignature() const {
    return {};
}

void UMaterial::Finalize(IAssetQuery* Query) {
}

void UMaterial::Serialize(FArchive& Ar) {
	UAsset::Serialize(Ar);
}
