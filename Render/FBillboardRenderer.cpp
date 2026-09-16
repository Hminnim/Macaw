#include "PCH.h"
#include "FBillboardRenderer.h"

#include "Render/Pipeline/UPipeline.h"
#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UTexture.h"
#include "Scene/Component/UBillboardComponent.h"
#include <algorithm>
#include <unordered_map>

bool FBillboardRenderer::Initialize(ID3D11Device* InDevice, uint32_t InitialCapacity)
{
	if (InDevice == nullptr || InitialCapacity == 0)
	{
		return false;
	}
	Device = InDevice;

	if (!ViewConstantBuffer.Initialize(Device))
	{
		return false;
	}

	return EnsureCapacity(InitialCapacity);
}

void FBillboardRenderer::Render(ID3D11DeviceContext* Context, const TArray<FBillboardProbe>& BillboardProbe, const CameraProbe& Camera, FAssetRegistry* AssetRegistry)
{
	if (Context == nullptr || Device == nullptr || BillboardProbe.empty())
	{
		return;
	}

	FMatrix CameraWorld{};
	if (!Camera.View.TryInverse(CameraWorld))
	{
		return;
	}

	FBillboardViewConstans Constants
	{
		.ViewProjection = Camera.ViewProjection,
		.CameraWorld = CameraWorld
	};

	if (!ViewConstantBuffer.SetGraphicsRoot32BitConstants(Constants, 0))
	{
		return;
	}
	ViewConstantBuffer.Bind(Context, 0, EGraphicsShaderStage::Geometry);

	// Release Vertex buffer, Index buffer
	UINT Stride = 0;
	UINT Offset = 0;
	ID3D11Buffer* NullBuffer = nullptr;
	Context->IASetVertexBuffers(0, 1, &NullBuffer, &Stride, &Offset);
	Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	// Pipeline, Texture Batch
	struct FBatchKey
	{
		FAssetHandle PipelineHandle;
		FAssetHandle TextureHandle;
		bool operator==(const FBatchKey& Other) const = default;
	};

	struct FBatchKeyHash
	{
		size_t operator()(const FBatchKey& Key) const noexcept
		{
			return std::hash<uint32_t>{}(Key.PipelineHandle.ID) ^ (std::hash<uint32_t>{}(Key.TextureHandle.ID) << 1);
		}
	};

	std::unordered_map<FBatchKey, TArray<FBillboardData>, FBatchKeyHash> Batches;
	for (const FBillboardProbe& Probe : BillboardProbe)
	{
		if (!Probe.PipelineHandle || !Probe.TextureHandle)
		{
			continue;
		}
		Batches[{ Probe.PipelineHandle, Probe.TextureHandle }].push_back(FBillboardData{
			.World = Probe.World,
			.Size = Probe.Size,
			.UVMin = Probe.UVMin,
			.UVMax = Probe.UVMax,
			.Pad = FVector2{ 0.0f, 0.0f },
			.Color = Probe.Color
			});
	}
	
	for (auto& [Key, InstanceArray] : Batches)
	{
		UPipeline* Pipeline = AssetRegistry->ResolveAsset<UPipeline>(Key.PipelineHandle);
		UTexture* Texture = AssetRegistry->ResolveAsset<UTexture>(Key.TextureHandle);
		if (Pipeline == nullptr || Texture == nullptr || InstanceArray.empty())
		{
			continue;
		}
		const uint32_t InstanceCount = static_cast<uint32_t>(InstanceArray.size());
		if (!EnsureCapacity(InstanceCount))
		{
			continue;
		}
		
		const uint32_t ByteSize = InstanceCount * sizeof(FBillboardData);
		if (!InstanceBuffer.WriteDiscard(Context, InstanceArray.data(), ByteSize))
		{
			continue;
		}
		
		Pipeline->Bind(Context);
		
		ID3D11ShaderResourceView* BufferSRV = InstanceBufferSRV.Get();
		Context->GSSetShaderResources(0, 1, &BufferSRV);		
		ID3D11ShaderResourceView* TextureSRV = Texture->GetSRV();
		Context->PSSetShaderResources(3, 1, &TextureSRV);
		Context->DrawInstanced(1, InstanceCount, 0, 0);
	}
}

bool FBillboardRenderer::EnsureCapacity(uint32 RequiredCapacity)
{
	if (RequiredCapacity <= InstanceCapacity)
	{
		return true;
	}

	uint32 NewCapacity = std::max(InstanceCapacity, 1u);

	while (NewCapacity < RequiredCapacity)
	{
		NewCapacity *= 2;
	}

	FGraphicsBufferDescription Description{};
	Description.ByteSize = NewCapacity * sizeof(FBillboardData);
	Description.Stride = sizeof(FBillboardData);
	Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Description.Usage = D3D11_USAGE_DYNAMIC;

	FGraphicsBuffer NewBuffer{};
	if (!NewBuffer.Initialize(Device, Description))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.NumElements = NewCapacity;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> NewSRV;
	HRESULT Hr = Device->CreateShaderResourceView(NewBuffer.GetBuffer(), &SRVDesc, NewSRV.GetAddressOf());
	if (FAILED(Hr))
	{
		return false;
	}

	InstanceBuffer = std::move(NewBuffer);
	InstanceBufferSRV = std::move(NewSRV);
	InstanceCapacity = NewCapacity;

	return true;
}
