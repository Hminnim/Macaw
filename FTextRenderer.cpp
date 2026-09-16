#include "PCH.h"
#include "FTextRenderer.h"

#include "Render/Pipeline/UPipeline.h"

#include "../Core/Asset/FAssetRegistry.h"
#include "../Core/Asset/UFont.h"

#include <algorithm>
#include <limits>

bool FTextRenderer::Initialize(ID3D11Device* InDevice, uint32_t InitialCapacity)
{
	if (InDevice == nullptr || InitialCapacity == 0)
	{
		return false;
	}
	Device = InDevice;
	if (!TextConstants.Initialize(Device))
	{
		return false;
	}
	return EnsureCapacity(InitialCapacity);
}

bool FTextRenderer::EnsureCapacity(uint32_t RequiredCapacity)
{
	if (RequiredCapacity <= VertexCapacity)
	{
		return true;
	}
	uint32_t NewCapacity = std::max(VertexCapacity, 1u);
	while (NewCapacity < RequiredCapacity)
	{
		NewCapacity *= 2;
	}
	if (NewCapacity > std::numeric_limits<uint32_t>::max() / sizeof(FTextVertex))
	{
		return false;
	}
	FGraphicsBufferDescription Description{};
	Description.ByteSize = NewCapacity * sizeof(FTextVertex);
	Description.Usage = D3D11_USAGE_DYNAMIC;
	Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	FGraphicsBuffer NewBuffer{};
	if (!NewBuffer.Initialize(Device, Description)) 
	{
		return false;
	}
	VertexBuffer = std::move(NewBuffer);
	VertexCapacity = NewCapacity;
	return true;
}

void FTextRenderer::Render(ID3D11DeviceContext* Context, const TArray<FTextProbe>& TextProbes, const CameraProbe& Camera, FAssetRegistry* AssetRegistry)
{
	if (Context == nullptr || Device == nullptr || TextProbes.empty())
	{
		return;
	}
	FMatrix CameraWorld{};
	if (!Camera.View.TryInverse(CameraWorld))
	{
		return;
	}
	for (const FTextProbe& Probe : TextProbes)
	{
		if (Probe.Vertices.empty())
		{
			continue;
		}
		UFont* Font = AssetRegistry->ResolveAsset<UFont>(Probe.FontHandle);
		if (Font == nullptr)
		{
			continue;
		}
		Font->FlushAtlas(Context);
		ID3D11ShaderResourceView* AtlasSRV = Font->GetAtlasSRV();
		if (AtlasSRV == nullptr)
		{
			if (AtlasSRV == nullptr)
			{
				continue;
			}
		}
		UPipeline* PipeLine = AssetRegistry->ResolveAsset<UPipeline>(Probe.PipelineHandle);
		if (PipeLine == nullptr)
		{
			continue;
		}
		const uint32_t VertexCount = static_cast<uint32_t>(Probe.Vertices.size());
		const uint32_t VertexByteSize = VertexCount * sizeof(FTextVertex);
		if (!EnsureCapacity(VertexCount))
		{
			continue;
		}
		if (!VertexBuffer.WriteDiscard(Context, Probe.Vertices.data(), VertexByteSize))
		{
			continue;
		}
		PipeLine->Bind(Context);
		ID3D11Buffer* Buffer = VertexBuffer.GetBuffer();
		UINT Stride = sizeof(FTextVertex);
		UINT Offset = 0;
		Context->IASetVertexBuffers(0,1,&Buffer,&Stride,&Offset);
		Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		Context->PSSetShaderResources(3, 1, &AtlasSRV);
		FTextConstants Constants{
			.World = Probe.World,
			.ViewwProjection = Camera.ViewProjection,
			.CameraWorld = CameraWorld,
			.Color = Probe.Color
		};
		if (!TextConstants.SetGraphicsRoot32BitConstants(Constants, 0))
		{
			continue;
		}
		if (!TextConstants.Bind(Context, 0, EGraphicsShaderStage::Geometry | EGraphicsShaderStage::Pixel))
		{
			continue;
		}
		Context->Draw(VertexCount, 0);
	}
}
