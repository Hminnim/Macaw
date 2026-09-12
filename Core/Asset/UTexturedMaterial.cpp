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

FMaterialChunkSignature UTexturedMaterial::BuildChunkSignature() const {
	FMaterialChunkSignatureBuilder Builder{};
	Builder.AddTexture(TextureHandle);
	return Builder.Build();
}

void UTexturedMaterial::Finalize(IAssetQuery* Query) {
	UAsset* Asset = Query->GetUAsset(TextureName);
	const bool bValidTexture = Asset != nullptr && Asset->GetTypeInfo()->IsA(UTexture::StaticTypeInfo());
	ErrorHandler::Report(!bValidTexture, "[ UTexturedMaterial ]", "Failed to resolve texture asset: " + TextureName, ErrorHandler::EErrorLevel::Critical);

	TextureHandle = bValidTexture ? Query->GetAsset(TextureName) : FAssetHandle{};
}

void UTexturedMaterial::Serialize(FArchive& Ar) {
	UMaterial::Serialize(Ar);
}
