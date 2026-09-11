#include "PCH.h"
#include "UTexture.h"

#include "FAssetMetadataParser.h"
#include "../../ErrorHandler.h"

#include <DirectXTex.h>

void UTexture::Initialize(ID3D11Device* device, const std::filesystem::path& metaData) {
	UAsset::Initialize(device, metaData);

	FAssetMetadataParser Parser;
	DirectX::ScratchImage SourceImage{};
	DirectX::ScratchImage GeneratedMipChain{};
	DirectX::TexMetadata SourceImageMetaData{};
	Parser.Load(metaData);
	auto path = Parser.ResolvePath("FilePath");


	if (path.extension() == ".dds" or path.extension() == ".DDS") {
		ErrorHandler::ReportHRESULT(DirectX::LoadFromDDSFile(path.wstring().c_str(), DirectX::DDS_FLAGS_NONE, &SourceImageMetaData, SourceImage), "[ UTexture ]", "Failed to load DDS texture: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}
	else if (path.extension() == ".tga" or path.extension() == ".TGA") {
		ErrorHandler::ReportHRESULT(DirectX::LoadFromTGAFile(path.wstring().c_str(), &SourceImageMetaData, SourceImage), "[ UTexture ]", "Failed to load TGA texture: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}
	else if (path.extension() == ".bmp" or path.extension() == ".BMP"
		or path.extension() == ".png" or path.extension() == ".PNG"
		or path.extension() == ".gif" or path.extension() == ".GIF"
		or path.extension() == ".tif" or path.extension() == ".TIF"
		or path.extension() == ".tiff" or path.extension() == ".TIFF"
		or path.extension() == ".jpg" or path.extension() == ".JPG"
		or path.extension() == ".jpeg" or path.extension() == ".JPEG") {
		ErrorHandler::ReportHRESULT(DirectX::LoadFromWICFile(path.wstring().c_str(), DirectX::WIC_FLAGS_NONE, &SourceImageMetaData, SourceImage), "[ UTexture ]", "Failed to load WIC texture: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}
	else if (path.extension() == ".hdr" or path.extension() == ".HDR") {
		ErrorHandler::ReportHRESULT(DirectX::LoadFromHDRFile(path.wstring().c_str(), &SourceImageMetaData, SourceImage), "[ UTexture ]", "Failed to load HDR texture: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}
	else {
		ErrorHandler::Report("[ UTexture ]", "Unsupported texture format: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}

	const DirectX::Image* Images = SourceImage.GetImages();
	size_t ImageCount = SourceImage.GetImageCount();
	const DirectX::TexMetadata* ImageMetaData = &SourceImageMetaData;

	if (SourceImageMetaData.mipLevels == 1) {
		const HRESULT Result = DirectX::GenerateMipMaps(
			Images,
			ImageCount,
			SourceImageMetaData,
			DirectX::TEX_FILTER_FANT,
			0,
			GeneratedMipChain);
		ErrorHandler::ReportHRESULT(Result, "[ UTexture ]", "Failed to generate mip maps: " + path.string(), ErrorHandler::EErrorLevel::Critical);

		if (FAILED(Result)) {
			return;
		}

		Images = GeneratedMipChain.GetImages();
		ImageCount = GeneratedMipChain.GetImageCount();
		ImageMetaData = &GeneratedMipChain.GetMetadata();
	}

	ErrorHandler::ReportHRESULT(
		DirectX::CreateShaderResourceView(device, Images, ImageCount, *ImageMetaData, ShaderResourceView.ReleaseAndGetAddressOf()),
		"[ UTexture ]",
		"Failed to create texture shader resource view: " + path.string(),
		ErrorHandler::EErrorLevel::Critical);

}

void UTexture::Serialize(FArchive& Ar) {
	UAsset::Serialize(Ar);
}
