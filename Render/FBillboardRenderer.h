#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "FMath.h"
#include "STL.h"
#include "Core/Base/FRenderProbe.h"
#include "Core/Buffer/FGraphicsBuffer.h"
#include "Core/Buffer/TGraphicsRootConstants.h"

class FAssetRegistry;

class FBillboardRenderer
{
private:
	struct FBillboardViewConstans
	{
		FMatrix ViewProjection{};
		FMatrix CameraWorld{};
	};

	static_assert(sizeof(FBillboardViewConstans) == sizeof(uint32) * 32);

public:
	FBillboardRenderer() = default;
	~FBillboardRenderer() = default;

	bool Initialize(ID3D11Device* InDevice, uint32_t InitialCapacity = 256);
	void Render(ID3D11DeviceContext* Context, const TArray<FBillboardProbe>& BillboardProbe, const CameraProbe& Camera, FAssetRegistry* AssetRegistry);

private:
	bool EnsureCapacity(uint32 RequiredCapacity);

private:
	ID3D11Device* Device = nullptr;

	FGraphicsBuffer InstanceBuffer{};
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> InstanceBufferSRV;
	uint32 InstanceCapacity = 0;
	TGraphicsRootConstants<32> ViewConstantBuffer;
};