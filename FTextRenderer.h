#pragma once

#include <d3d11.h>

#include "FMath.h"
#include "STL.h"
#include "../Core/Base/FRenderProbe.h"
#include "../Core/Buffer/FGraphicsBuffer.h"
#include "../Core/Buffer/TGraphicsRootConstants.h"

class FAssetRegistry;

class FTextRenderer
{
private:
	struct FTextConstants 
	{
		FMatrix World{};
		FMatrix ViewwProjection{};
		FMatrix CameraWorld{};
		FVector4 Color{ 1.0f,1.0f,1.0f,1.0f };
	};

	static_assert(sizeof(FTextConstants) == sizeof(uint32_t) * 52);
public:
	bool Initialize(ID3D11Device* InDevice, uint32_t InitialCapacity = 256);
	void Render(ID3D11DeviceContext* Context, const TArray<FTextProbe>& TextProbes, const CameraProbe& Camera, FAssetRegistry* AssetRegistry);
private:
	bool EnsureCapacity(uint32_t ReauiredCapacity);
	ID3D11Device* Device = nullptr;
	FGraphicsBuffer VertexBuffer{};
	uint32_t VertexCapacity = 0;
	TGraphicsRootConstants<52> TextConstants{};
};