#include "PCH.h"
#include "UTexture.h"

#include "FAssetMetadataParser.h"
#include "../../ErrorHandler.h"

void UTexture::Initialize(ID3D11Device* device, const std::filesystem::path& metaData) {
	UAsset::Initialize(device, metaData);

	FAssetMetadataParser Parser;
	Parser.Load(metaData);
	auto path = Parser.ResolvePath("FilePath");


	if (path.extension() == ".tga" or path.extension() == ".TGA") {
		ErrorHandler::ReportHRESULT(DirectX::LoadFromTGAFile(path.wstring().c_str(), &SourceImageMetaData, SourceImage), "[ UTexture ]", "Failed to load TGA texture: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}
	else if (path.extension() == ".png" or path.extension() == ".PNG") {
		ErrorHandler::ReportHRESULT(DirectX::LoadFromWICFile(path.wstring().c_str(), DirectX::WIC_FLAGS_NONE, &SourceImageMetaData, SourceImage), "[ UTexture ]", "Failed to load PNG texture: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}
	else if (path.extension() == ".hdr" or path.extension() == ".HDR") {
		ErrorHandler::ReportHRESULT(DirectX::LoadFromHDRFile(path.wstring().c_str(), &SourceImageMetaData, SourceImage), "[ UTexture ]", "Failed to load HDR texture: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}
	else {
		ErrorHandler::Report("[ UTexture ]", "Unsupported texture format: " + path.string(), ErrorHandler::EErrorLevel::Critical);
	}

	Profile.Format = SourceImageMetaData.format;
	Profile.Width = static_cast<uint32>(SourceImageMetaData.width);
	Profile.Height = static_cast<uint32>(SourceImageMetaData.height);
}

const byte* UTexture::GetSourceImageData() const {
	auto raw = SourceImage.GetImage(0, 0, 0)->pixels;
	return reinterpret_cast<const byte*>(raw);
}

const size_t UTexture::GetSourceImageDataSize() const {
	return SourceImage.GetImage(0, 0, 0)->slicePitch; 
}

const uint32 UTexture::GetSourceImageRowPitch() const {
	return static_cast<uint32>(SourceImage.GetImage(0, 0, 0)->rowPitch);
}

void UTexture::Serialize(FArchive& Ar) {
	UAsset::Serialize(Ar);
}
