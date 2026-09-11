#include "PCH.h"
#include "FTextureArrayPool.h"
#include "../../ErrorHandler.h"

bool FTextureArrayPool::Initialize(ID3D11Device* Device, const FTextureProfile& InProfile, uint16 InSliceCapacity) {
	Reset();

	if (Device == nullptr || !InProfile.IsValid() || InSliceCapacity == 0 || InSliceCapacity > MAX_TEXTURE_ARRAY_SLICE_COUNT) {
		return false;
	}

	UINT FormatSupport{};

	if (FAILED(Device->CheckFormatSupport(InProfile.Format, &FormatSupport))) {
		return false;
	}

	constexpr UINT RequiredFormatSupport = D3D11_FORMAT_SUPPORT_TEXTURE2D
		| D3D11_FORMAT_SUPPORT_SHADER_SAMPLE
		| D3D11_FORMAT_SUPPORT_RENDER_TARGET
		| D3D11_FORMAT_SUPPORT_MIP_AUTOGEN;

	if ((FormatSupport & RequiredFormatSupport) != RequiredFormatSupport) {
		return false;
	}

	Profile = InProfile;
	MipLevelCount = CalculateMipLevelCount(Profile.Width, Profile.Height);
	SliceCapacity = InSliceCapacity;

	if (!CreateTexture(Device) || !CreateSRV(Device)) {
		Reset();
		return false;
	}

	FreeSlices.reserve(SliceCapacity);
	AllocatedSlices.resize(SliceCapacity, 0);

	for (uint16 SliceId = 0; SliceId < SliceCapacity; ++SliceId) {
		FreeSlices.push_back(static_cast<uint16>(SliceCapacity - SliceId - 1));
	}

	return true;
}

std::optional<uint16> FTextureArrayPool::AllocateSlice() {
	if (FreeSlices.empty()) {
		return std::nullopt;
	}

	const uint16 SliceId = FreeSlices.back();
	FreeSlices.pop_back();

	AllocatedSlices[SliceId] = 1;
	++AllocatedSliceCount;

	return SliceId;
}

void FTextureArrayPool::ReleaseSlice(uint16 SliceId) {
	if (!IsSliceAllocated(SliceId)) {
		return;
	}

	AllocatedSlices[SliceId] = 0;
	--AllocatedSliceCount;

	FreeSlices.push_back(SliceId);
}

bool FTextureArrayPool::UploadSlice(ID3D11DeviceContext* DeviceContext, uint16 SliceId, std::span<const FTextureData> SourceMips) {
	if (DeviceContext == nullptr || !IsSliceAllocated(SliceId) || SourceMips.empty() || SourceMips.size() > MipLevelCount) {
		return false;
	}

	for (const FTextureData& SourceMip : SourceMips) {
		if (SourceMip.Data == nullptr || SourceMip.RowPitch == 0) {
			return false;
		}
	}

	if (SourceMips.size() == MipLevelCount) {
		for (uint32 MipLevel = 0; MipLevel < MipLevelCount; ++MipLevel) {
			const FTextureData& SourceMip = SourceMips[MipLevel];
			const uint32 Subresource = D3D11CalcSubresource(MipLevel, SliceId, MipLevelCount);

			DeviceContext->UpdateSubresource(Texture.Get(), Subresource, nullptr, SourceMip.Data, SourceMip.RowPitch, SourceMip.DepthPitch);
		}

		return true;
	}

	const FTextureData& Mip0 = SourceMips.front();
	const uint32 Subresource = D3D11CalcSubresource(0, SliceId, MipLevelCount);

	DeviceContext->UpdateSubresource(Texture.Get(), Subresource, nullptr, Mip0.Data, Mip0.RowPitch, Mip0.DepthPitch);

	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Texture->GetDevice(Device.GetAddressOf());

	return GenerateMips(Device.Get(), DeviceContext, SliceId);
}



std::optional<uint16> FTextureArrayPool::AppendImage(ID3D11DeviceContext* DeviceContext, const FTextureData& SourceMips) {
	if (DeviceContext == nullptr or SourceMips.Data == nullptr or SourceMips.RowPitch == 0) {
		return std::nullopt;
	}

	auto slice = AllocateSlice();

	if (slice == std::nullopt) {
		return std::nullopt;
	}

	const uint32 Subresource = D3D11CalcSubresource(0, slice.value(), MipLevelCount);

	DeviceContext->UpdateSubresource(Texture.Get(), Subresource, nullptr, SourceMips.Data, SourceMips.RowPitch, SourceMips.DepthPitch);

	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	DeviceContext->GetDevice(Device.GetAddressOf());
	ErrorHandler::Report(not GenerateMips(Device.Get(), DeviceContext, slice.value()), "[ TextureArrayPool ]",  "Failed to Generate Mip Maps", ErrorHandler::EErrorLevel::Critical);

	return slice; 
}

bool FTextureArrayPool::ReplaceImage(ID3D11DeviceContext* DeviceContext, const FTextureLocation& Location, const FTextureData& SourceMips) {
	if (DeviceContext == nullptr || !IsSliceAllocated(Location.SliceId) || SourceMips.Data == nullptr || SourceMips.RowPitch == 0) {
		return false;
	}

	const uint32 Subresource = D3D11CalcSubresource(0, Location.SliceId, MipLevelCount);

	DeviceContext->UpdateSubresource(Texture.Get(), Subresource, nullptr, SourceMips.Data, SourceMips.RowPitch, SourceMips.DepthPitch);
	
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	DeviceContext->GetDevice(Device.GetAddressOf());
	ErrorHandler::Report(not GenerateMips(Device.Get(), DeviceContext, Location.SliceId), "[ TextureArrayPool ]", "Failed to Generate Mip Maps", ErrorHandler::EErrorLevel::Critical);

	return true;
}

void FTextureArrayPool::Reset() {
	Profile = {};
	MipLevelCount = 0;
	SliceCapacity = 0;
	AllocatedSliceCount = 0;

	FreeSlices.clear();
	AllocatedSlices.clear();

	Texture.Reset();
	SRV.Reset();
}

bool FTextureArrayPool::CreateTexture(ID3D11Device* Device) {
	D3D11_TEXTURE2D_DESC TextureDesc{};
	TextureDesc.Width = Profile.Width;
	TextureDesc.Height = Profile.Height;
	TextureDesc.MipLevels = MipLevelCount;
	TextureDesc.ArraySize = SliceCapacity;
	TextureDesc.Format = Profile.Format;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	return SUCCEEDED(Device->CreateTexture2D(&TextureDesc, nullptr, Texture.GetAddressOf()));
}

bool FTextureArrayPool::CreateSRV(ID3D11Device* Device) {
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = Profile.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = MipLevelCount;
	SRVDesc.Texture2DArray.FirstArraySlice = 0;
	SRVDesc.Texture2DArray.ArraySize = SliceCapacity;

	return SUCCEEDED(Device->CreateShaderResourceView(Texture.Get(), &SRVDesc, SRV.GetAddressOf()));
}

bool FTextureArrayPool::GenerateMips(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, uint16 SliceId) {
	if (Device == nullptr || DeviceContext == nullptr) {
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = Profile.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = MipLevelCount;
	SRVDesc.Texture2DArray.FirstArraySlice = SliceId;
	SRVDesc.Texture2DArray.ArraySize = 1;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SliceSRV;

	if (FAILED(Device->CreateShaderResourceView(Texture.Get(), &SRVDesc, SliceSRV.GetAddressOf()))) {
		return false;
	}

	DeviceContext->GenerateMips(SliceSRV.Get());

	return true;
}

bool FTextureArrayPool::IsSliceAllocated(uint16 SliceId) const {
	return SliceId < SliceCapacity && AllocatedSlices[SliceId] != 0;
}

uint32 FTextureArrayPool::CalculateMipLevelCount(uint32 Width, uint32 Height) {
	uint32 LargestDimension = Width > Height ? Width : Height;
	uint32 MipLevelCount = 1;

	while (LargestDimension > 1) {
		LargestDimension >>= 1;
		++MipLevelCount;
	}

	return MipLevelCount;
}
