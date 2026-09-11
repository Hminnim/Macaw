#pragma once

#include "FTextureProfile.h"

#include <d3d11.h>
#include <wrl/client.h>

#include <optional>
#include <span>

inline constexpr uint16 MAX_TEXTURE_ARRAY_SLICE_COUNT = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;

struct FTextureData {
	const void* Data{ nullptr };
	uint32 RowPitch{ 0 };
	uint32 DepthPitch{ 0 };
};

class FTextureArrayPool {
public:
	FTextureArrayPool() = default;
	~FTextureArrayPool() = default;

	FTextureArrayPool(const FTextureArrayPool&) = delete;
	FTextureArrayPool& operator=(const FTextureArrayPool&) = delete;

	FTextureArrayPool(FTextureArrayPool&&) = delete;
	FTextureArrayPool& operator=(FTextureArrayPool&&) = delete;

public:
	bool Initialize(ID3D11Device* Device, const FTextureProfile& InProfile, uint16 InSliceCapacity = MAX_TEXTURE_ARRAY_SLICE_COUNT);

	bool UploadSlice(ID3D11DeviceContext* DeviceContext, uint16 SliceId, std::span<const FTextureData> SourceMips);

	std::optional<uint16> AppendImage(ID3D11DeviceContext* DeviceContext, const FTextureData& SourceMips);
	bool ReplaceImage(ID3D11DeviceContext* DeviceContext, const FTextureLocation& Location, const FTextureData& SourceMips);

	const FTextureProfile& GetProfile() const { return Profile; }
	uint32 GetMipLevelCount() const { return MipLevelCount; }
	uint16 GetSliceCapacity() const { return SliceCapacity; }
	uint16 GetAllocatedSliceCount() const { return AllocatedSliceCount; }

	ID3D11Texture2D* GetTexture() const { return Texture.Get(); }
	ID3D11ShaderResourceView* GetSRV() const { return SRV.Get(); }

	void Reset();

private:
	std::optional<uint16> AllocateSlice();
	void ReleaseSlice(uint16 SliceId);

	bool CreateTexture(ID3D11Device* Device);
	bool CreateSRV(ID3D11Device* Device);
	bool GenerateMips(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, uint16 SliceId);
	bool IsSliceAllocated(uint16 SliceId) const;

	static uint32 CalculateMipLevelCount(uint32 Width, uint32 Height);

private:
	FTextureProfile Profile{};
	uint32 MipLevelCount{ 0 };
	uint16 SliceCapacity{ 0 };
	uint16 AllocatedSliceCount{ 0 };

	TArray<uint16> FreeSlices{};
	TArray<uint8> AllocatedSlices{};

	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture{};
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV{};
};
