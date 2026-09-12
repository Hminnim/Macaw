#include "PCH.h"
#include "UColorMaterial.h"

#include <cstring>

#include "FAssetMetadataParser.h"
#include "../../ErrorHandler.h"


void UColorMaterial::Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) {
    UMaterial::Initialize(Device, metaData);

	FAssetMetadataParser MetadataParser{};

	ErrorHandler::Report(not MetadataParser.Load(AssetMetaDataPath), " [ UColorMaterial ]", "Failed to load metadata", ErrorHandler::EErrorLevel::Critical);

	Color = MetadataParser.GetOr("Color", FColor4{ 1.0f, 1.0f, 1.0f, 1.0f });

}

void UColorMaterial::BuildGPUData(FMaterialGPUSlot& OutSlot) const {
    FColorMaterialGPUData Data{};
    Data.Color = Color;

    std::memcpy(OutSlot.Data.data(), &Data, sizeof(FColorMaterialGPUData));
}

void UColorMaterial::Finalize(IAssetQuery* Query)
{
}

void UColorMaterial::Serialize(FArchive& Ar) {
	UMaterial::Serialize(Ar);
}

void UColorMaterial::SetColor(const FVector4& InColor) {
    Color = InColor;
    UMaterial::MarkGPUDataDirty();
}