#include "PCH.h"

#include "Renderer.h"
#include "../ErrorHandler.h"

#include "Pipeline/UPipeline.h"
#include "../Core/Asset/UTexture.h"

#include <ranges>
#include <range/v3/view/chunk_by.hpp>



FRenderer::~FRenderer() {

}

void FRenderer::Create(HWND WindowHandle, UINT width, UINT height) {
	WindowInfoWriter.Emplace(RenderWindowInfo{
		.ScreenWidth = width,
		.ScreenHeight = height,
		.Viewport = {}
	});

	FRenderer::CreateDeviceAndSwapChain(WindowHandle);
	auto BackBuffer = std::make_unique<FSceneRenderSurface>();
	BackBuffer->InitializeSwapChain(Device.Get(), SwapChain.Get());
	BackBufferSurface = std::move(BackBuffer);
	auto Scene = std::make_unique<FSceneRenderSurface>();
	Scene->InitializeOffscreen(Device.Get(), width, height);
	SceneSurface = std::move(Scene);
	FRenderer::CreateSamplerStates();

	ModelContextArray.Initialize(Device.Get(), DeviceContext.Get(), 128);
	FrameContexts.reserve(128);
	RootConstants.Initialize(Device.Get());
	TextRenderer.Initialize(Device.Get(),256);

#ifdef _DEBUG
	Device.As(&DebugInterface);
#endif
}

void FRenderer::BeginSceneRender() {
	SceneSurface->Bind(DeviceContext.Get());
	SceneSurface->Clear(DeviceContext.Get(), ClearColor);
}

void FRenderer::BeginUiRender() {
	BackBufferSurface->Bind(DeviceContext.Get());
	BackBufferSurface->Clear(DeviceContext.Get(), ClearColor);
}

void FRenderer::BindSamplerStates() {
	std::array<ID3D11SamplerState*, 6> RawSamplerStates{};
	std::ranges::transform(SamplerStates, RawSamplerStates.begin(), [](const auto& Sampler) {
		return Sampler.Get();
		});
	DeviceContext->PSSetSamplers(0, static_cast<UINT>(RawSamplerStates.size()), RawSamplerStates.data());
}

void FRenderer::EndFrame() {
	SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void FRenderer::RenderScene(FRenderProbe& Probe) {
	if (AssetRegistry != nullptr) {
		AssetRegistry->GetMaterialBuffer().Flush(DeviceContext.Get());
	}

	RenderActorList(Probe.ActorProbes,Probe.MainCameraProbe);
	RenderOutline(Probe.ActorProbes, Probe.MainCameraProbe);

}


void FRenderer::RenderGizmos(FRenderProbe& Probe) {
	if (Probe.GizmoProbes.empty()) {
		return;
	}

	SceneSurface->ClearDepth(DeviceContext.Get());

	RenderActorList(Probe.GizmoProbes,Probe.MainCameraProbe);
}

void FRenderer::RenderOutline(const TArray<FActorProbe>& ActorProbes, const CameraProbe& MainCameraProbe) {
	TArray<FActorProbe> OutlineProbes;

	for (const FActorProbe& ActorProbe : ActorProbes)
	{
		if ((ActorProbe.Flags & static_cast<uint32>(ERenderObjectFlags::Selected)) != 0)
		{
			OutlineProbes.push_back(ActorProbe);
		}
	}

	if (OutlineProbes.empty())
	{
		return;
	}

	RenderActorList(OutlineProbes, MainCameraProbe, true);
}

void FRenderer::RenderActorList(TArray<FActorProbe>& ActorProbes, const CameraProbe& MainCameraProbe, bool bOutline) {
    if (ActorProbes.empty() || AssetRegistry == nullptr) {
        return;
    }

 
    std::erase_if(ActorProbes, [this](const FActorProbe& Probe) {
        return AssetRegistry->ResolveAsset<UMaterial>(Probe.MaterialHandle) == nullptr ||
            AssetRegistry->ResolveAsset<UPipeline>(Probe.PipelineHandle) == nullptr ||
            AssetRegistry->ResolveAsset<UMesh>(Probe.MeshHandle) == nullptr;
    });

    if (ActorProbes.empty()) {
        return;
    }

	auto GetRenderChunkKey = [this](const FActorProbe& Data) {
		const FMaterialChunkSignature Signature = AssetRegistry->ResolveAsset<UMaterial>(Data.MaterialHandle)->BuildChunkSignature();
		return TTuple{
			Data.PipelineHandle.ID,
			Data.PipelineHandle.Generation,
			Signature.TextureFieldCount,
			Signature.TextureHandles,
			Data.MeshHandle.ID,
			Data.MeshHandle.Generation
			};
		};

	std::ranges::sort(ActorProbes, {}, GetRenderChunkKey);

	auto Groups = ActorProbes | ranges::views::chunk_by([&GetRenderChunkKey](const FActorProbe& A, const FActorProbe& B) {
		return GetRenderChunkKey(A) == GetRenderChunkKey(B);
		});

	FrameContexts.clear();
	FrameContexts.reserve(ActorProbes.size());

	std::ranges::transform(Groups | std::views::join, std::back_inserter(FrameContexts), [&](const auto& AC) {
		return ModelContext{
			.World = AC.World,
			.MaterialIndex = AssetRegistry->ResolveAsset<UMaterial>(AC.MaterialHandle)->GetGPUIndex(),
			.Flags = AC.Flags
		};
	});

	ID3D11ShaderResourceView* NullModelContext = nullptr;
	DeviceContext->VSSetShaderResources(0, 1, &NullModelContext);
	DeviceContext->PSSetShaderResources(0, 1, &NullModelContext);
	if (!ModelContextArray.UploadDiscard(Device.Get(), DeviceContext.Get(), FrameContexts)) {
		return;
	}

	DeviceContext->VSSetShaderResources(0, 1, ModelContextArray.GetSRV());
	DeviceContext->PSSetShaderResources(0, 1, ModelContextArray.GetSRV());

	DeviceContext->VSSetShaderResources(1, 1, AssetRegistry->GetMaterialBuffer().GetSRV());
	DeviceContext->PSSetShaderResources(1, 1, AssetRegistry->GetMaterialBuffer().GetSRV());

	struct CameraData {
		FMatrix View;
		FMatrix Projection;
		FMatrix ViewProjection;
	};

	RootConstants.SetGraphicsRoot32BitConstants(CameraData{
		.View = MainCameraProbe.View,
		.Projection = MainCameraProbe.Projection,
		.ViewProjection = MainCameraProbe.ViewProjection
		}, 0);

	RootConstants.Bind(DeviceContext.Get(), 0, EGraphicsShaderStage::Graphics);
	uint32 InstanceCount{ 0 };
	FMaterialChunkSignature BoundTextureSet{};
	bool bTextureSetBound{ false };

	BindSamplerStates();

	for (auto g : Groups) {
		const FActorProbe& First = g.front();
		const FMaterialChunkSignature Signature = AssetRegistry->ResolveAsset<UMaterial>(First.MaterialHandle)->BuildChunkSignature();
		
		
		UPipeline* Pipeline = AssetRegistry->ResolveAsset<UPipeline>(First.PipelineHandle);
		if (bOutline) {
			Pipeline->SetRenderMode(ERenderMode::Outline);
		}
		
		UMesh* Mesh = AssetRegistry->ResolveAsset<UMesh>(First.MeshHandle);
		
		Pipeline->Bind(DeviceContext.Get());

		if (!bTextureSetBound || BoundTextureSet != Signature) {
			std::array<ID3D11ShaderResourceView*, MAX_MATERIAL_TEXTURE_FIELDS> TextureSRVs{};

			for (uint8 TextureFieldIndex = 0; TextureFieldIndex < Signature.TextureFieldCount; ++TextureFieldIndex) {
				UTexture* Texture = AssetRegistry->ResolveAsset<UTexture>(Signature.GetTextureHandle(TextureFieldIndex));
				TextureSRVs[TextureFieldIndex] = Texture != nullptr ? Texture->GetSRV() : nullptr;
			}

			if (Signature.TextureFieldCount > 0) {
				DeviceContext->PSSetShaderResources(2, Signature.TextureFieldCount, TextureSRVs.data());
			}

			BoundTextureSet = Signature;
			bTextureSetBound = true;
		}
	
		ID3D11Buffer* VertexBuffers[] = { 
			Mesh->GetVertexBuffer(EVertexAttribute::Position),
			Mesh->GetVertexBuffer(EVertexAttribute::Normal),
			Mesh->GetVertexBuffer(EVertexAttribute::UV)
		};

		uint32 Strides[] = { 
			Mesh->GetVertexStride(EVertexAttribute::Position),
			Mesh->GetVertexStride(EVertexAttribute::Normal),
			Mesh->GetVertexStride(EVertexAttribute::UV)
		};

		uint32 Offsets[] = { 0, 0, 0 };

		ID3D11Buffer* IndexBuffer { Mesh->GetIndexBuffer() };

		DeviceContext->IASetVertexBuffers(0, _countof(VertexBuffers), VertexBuffers, Strides, Offsets);
		DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		RootConstants.SetGraphicsRoot32BitConstant(InstanceCount, 48);

		RootConstants.Commit(DeviceContext.Get());

		DeviceContext->DrawIndexedInstanced(static_cast<uint32>(Mesh->GetIndices().size()), static_cast<uint32>(g.size()), 0, 0, 0);

		InstanceCount += static_cast<uint32>(g.size());
	}
}

void FRenderer::ReSize(uint32 width, uint32 height) {
	if (!SwapChain || width == 0 || height == 0) {
		return;
	}

	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	BackBufferSurface->Resize(Device.Get(), width, height);
}

void FRenderer::ResizeSceneSurface(uint32 Width, uint32 Height, float Left, float Top) {
	if (Width == 0 || Height == 0) {
		return;
	}

	SceneSurface->Resize(Device.Get(), Width, Height);
	WindowInfoWriter.Modify([&](RenderWindowInfo& Info) {
		Info.ScreenWidth = Width;
		Info.ScreenHeight = Height;
		Info.Viewport = { Left, Top, static_cast<float>(Width), static_cast<float>(Height), 0.0f, 1.0f };
	});
}

void FRenderer::Terminate() {
	DeviceContext->ClearState();

	if (SceneSurface != nullptr) {
		SceneSurface->Reset();
	}
	if (BackBufferSurface != nullptr) {
		BackBufferSurface->Reset();
	}
	SceneSurface.reset();
	BackBufferSurface.reset();
	SwapChain.Reset();
}

void FRenderer::ReportLiveObjects() const {
#ifdef _DEBUG
	DebugInterface->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
#endif 
}

void FRenderer::CreateDeviceAndSwapChain(HWND WindowHandle) {
	// 지원하는 Direct3D 기능 레벨을 정의
	D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

	// 스왑 체인 설정 구조체 초기화
	DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
	swapchaindesc.BufferDesc.Width = WindowInfoReader.Read().ScreenWidth; // 창 크기에 맞게 자동으로 설정
	swapchaindesc.BufferDesc.Height = WindowInfoReader.Read().ScreenHeight; // 창 크기에 맞게 자동으로 설정
	swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
	swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
	swapchaindesc.BufferCount = 2; // 더블 버퍼링
	swapchaindesc.OutputWindow = WindowHandle; // 렌더링할 창 핸들
	swapchaindesc.Windowed = TRUE; // 창 모드
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식
	swapchaindesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // 모드 전환 허용
	
#ifdef _DEBUG
	// Direct3D 장치와 스왑 체인을 생성
	ErrorHandler::ReportHRESULT(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
		featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
		&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext), "[ FRenderer ]", "Failed to create Direct3D device and swap chain.", ErrorHandler::EErrorLevel::Critical);
#else 
	ErrorHandler::ReportHRESULT(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT ,
		featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
		&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext), "[ FRenderer ]", "Failed to create Direct3D device and swap chain.", ErrorHandler::EErrorLevel::Critical);
#endif 
	// 생성된 스왑 체인의 정보 가져오기
	SwapChain->GetDesc(&swapchaindesc);

}

void FRenderer::CreateSamplerStates() {
	auto CreateSampler = [this](size_t Slot, const D3D11_SAMPLER_DESC& Description, const char* Name) {
		ErrorHandler::ReportHRESULT(
			Device->CreateSamplerState(&Description, SamplerStates[Slot].ReleaseAndGetAddressOf()),
			"[ FRenderer ]",
			std::string("Failed to create ") + Name + " sampler.",
			ErrorHandler::EErrorLevel::Critical);
		};

	auto MakeDescription = [](D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressMode) {
		D3D11_SAMPLER_DESC Description{};
		Description.Filter = Filter;
		Description.AddressU = AddressMode;
		Description.AddressV = AddressMode;
		Description.AddressW = AddressMode;
		Description.MipLODBias = 0.0f;
		Description.MaxAnisotropy = Filter == D3D11_FILTER_ANISOTROPIC ? 8 : 1;
		Description.ComparisonFunc = D3D11_COMPARISON_NEVER;
		Description.MinLOD = 0.0f;
		Description.MaxLOD = D3D11_FLOAT32_MAX;
		return Description;
		};

	CreateSampler(0, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP), "LinearWrap");
	CreateSampler(1, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP), "LinearClamp");
	CreateSampler(2, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP), "PointClamp");
	CreateSampler(3, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP), "PointWrap");
	CreateSampler(4, MakeDescription(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP), "AnisotropicWrap");

	D3D11_SAMPLER_DESC ShadowDescription = MakeDescription(
		D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_BORDER);
	ShadowDescription.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	ShadowDescription.BorderColor[0] = 1.0f;
	ShadowDescription.BorderColor[1] = 1.0f;
	ShadowDescription.BorderColor[2] = 1.0f;
	ShadowDescription.BorderColor[3] = 1.0f;
	CreateSampler(5, ShadowDescription, "ShadowCompare");
}

void FRenderer::RenderText(const FRenderProbe& Probe)
{
	if (AssetRegistry != nullptr)
	{
			TextRenderer.Render(DeviceContext.Get(),Probe.TextProbes,Probe.MainCameraProbe,AssetRegistry);
	}
}
