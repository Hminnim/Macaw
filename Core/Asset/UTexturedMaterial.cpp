#include "PCH.h"
#include "UTexturedMaterial.h"

#include "UTexture.h"
#include "FAssetMetadataParser.h"
#include "../../ErrorHandler.h"

void UTexturedMaterial::Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) {
	UMaterial::Initialize(Device, metaData);

	FAssetMetadataParser MetadataParser{};

	ErrorHandler::Report(not MetadataParser.Load(AssetMetaDataPath), " [ UTexturedMaterial ]", "Failed to load metadata", ErrorHandler::EErrorLevel::Critical);

	MetadataParser.TryGet("TextureName", TextureName);
	
}

void UTexturedMaterial::BuildGPUData(FMaterialGPUSlot& OutSlot) const {
	FTexturedMaterialGPUData Data{};
	Data.Location = PackTextureLocation(Location);

	std::memcpy(OutSlot.Data.data(), &Data, sizeof(FTexturedMaterialGPUData));
}

void UTexturedMaterial::Finalize(IAssetQuery* Query) {
	auto utex = Query->GetUAsset(TextureName); 
	
	ErrorHandler::Report(utex == nullptr, "[ UTexturedMaterial ]", "Failed to resolve texture asset: " + TextureName, ErrorHandler::EErrorLevel::Critical);

	auto tex = static_cast<UTexture*>(utex);
	
	Location = tex->GetLocation();
}

void UTexturedMaterial::Serialize(FArchive& Ar) {
	UMaterial::Serialize(Ar);
}
