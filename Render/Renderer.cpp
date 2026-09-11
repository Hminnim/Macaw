#include "PCH.h"

#include "Renderer.h"
#include "../ErrorHandler.h"

#include "Pipeline/UPipeline.h"

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
	FRenderer::CreateRTV();
	FRenderer::CreateDSV();

	ModelContextArray.Initialize(Device.Get(), DeviceContext.Get(), 128);
	RootConstants.Initialize(Device.Get());
}

void FRenderer::BeginFrame() {
	DeviceContext->ClearRenderTargetView(RenderTargetView.Get(), ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DeviceContext->OMSetRenderTargets(1, RenderTargetView.GetAddressOf(), DepthStencilView.Get());
	
	DeviceContext->RSSetViewports(1, &WindowInfoReader.Read().Viewport);
}

void FRenderer::EndFrame() {
	SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void FRenderer::RenderScene(FRenderProbe& Probe)
{
	RenderActorList(
		Probe.ActorProbes,
		Probe.MainCameraProbe
	);
}


void FRenderer::RenderGizmos(FRenderProbe& Probe)
{
	RenderActorList(
		Probe.GizmoProbes,
		Probe.MainCameraProbe
	);
}

void FRenderer::RenderActorList(TArray<FActorProbe>& ActorProbes, const CameraProbe& MainCameraProbe) {
	// 1. MeshHandle + PipelineHandle 로 정렬
	// 2. 정렬한 뒤 MeshHandle + PipelineHandle 이 같은 것 끼리 Batch 생성 
	// 3. Batch 순서대로 SRV Push Back  
	// 4. Batch 순서대로 InstanceDraw 호출

	if (ActorProbes.empty()) {
		return;
	}

	std::ranges::sort(ActorProbes, {}, [](const FActorProbe& Data){ return TTuple{Data.MeshHandle.ID, Data.MeshHandle.Generation, Data.PipelineHandle.ID, Data.PipelineHandle.Generation}; });

	auto Groups = ActorProbes | ranges::views::chunk_by([](const FActorProbe& A, const FActorProbe& B) {
		return A.MeshHandle == B.MeshHandle && A.PipelineHandle == B.PipelineHandle;
		});

	ModelContextArray.Clear();

	TArray<ModelContext> Contexts;
	Contexts.reserve(ActorProbes.size());

	std::ranges::transform(Groups | std::views::join, std::back_inserter(Contexts), [&](const auto& AC) {
		return ModelContext{
			.World = AC.World,
			.MaterialIndex = AssetRegistry->ResolveAsset<UMaterial>(AC.MaterialHandle)->GetGPUIndex(),
			.Flags = AC.Flags
		};
	});

	ModelContextArray.AddRange(Device.Get(), DeviceContext.Get(), Contexts);

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
	AssetRegistry->GetMaterialBuffer().Flush(DeviceContext.Get());

	uint32 InstanceCount{ 0 };

	for (auto g : Groups) {
		const FActorProbe& First = g.front();
		UPipeline* Pipeline = AssetRegistry->ResolveAsset<UPipeline>(First.PipelineHandle);
		UMesh* Mesh = AssetRegistry->ResolveAsset<UMesh>(First.MeshHandle);
		
		Pipeline->Bind(DeviceContext.Get());

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
	WindowInfoWriter.Modify([&](RenderWindowInfo& Info) {
		Info.ScreenWidth = width;
		Info.ScreenHeight = height;
		Info.Viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		}
	);

	// 최소화되었을 때는 0x0이 들어올 수 있음
	if (!SwapChain || width == 0 || height == 0) {
		return;
	}

	// ResizeBuffers 전에 백 버퍼를 참조하는 모든 리소스를 해제해야 함
	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

	RenderTargetView.Reset();
	BackBuffer.Reset();

	DepthStencilView.Reset();
	DepthStencilBuffer.Reset();

	DXGI_SWAP_CHAIN_DESC SwapChainDesc{};
	ErrorHandler::ReportHRESULT(SwapChain->GetDesc(&SwapChainDesc), "[ FRenderer ]", "Failed to get swap chain description.", ErrorHandler::EErrorLevel::Critical);

	ErrorHandler::ReportHRESULT(SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, SwapChainDesc.Flags), "[ FRenderer ]", "Failed to resize swap chain buffers.", ErrorHandler::EErrorLevel::Critical);

	const D3D11_VIEWPORT Viewport{
		0.0f,
		0.0f,
		static_cast<float>(width),
		static_cast<float>(height),
		0.0f,
		1.0f
	};

	WindowInfoWriter.Modify([&](RenderWindowInfo& Info) {
		Info.ScreenWidth = width;
		Info.ScreenHeight = height;
		Info.Viewport = Viewport;
		});

	CreateRTV();
	CreateDSV();

	DeviceContext->RSSetViewports(1, &Viewport);
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

	// Direct3D 장치와 스왑 체인을 생성
	ErrorHandler::ReportHRESULT(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
		featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
		&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext), "[ FRenderer ]", "Failed to create Direct3D device and swap chain.", ErrorHandler::EErrorLevel::Critical);

	// 생성된 스왑 체인의 정보 가져오기
	SwapChain->GetDesc(&swapchaindesc);

	// 뷰포트 정보 설정
	WindowInfoWriter.Modify([&](RenderWindowInfo& Info) {
		Info.Viewport = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
	});
}

void FRenderer::CreateRTV() {
	// 스왑 체인으로부터 백 버퍼 텍스처 가져오기
	SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer.GetAddressOf()));

	// 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

	ErrorHandler::ReportHRESULT(Device->CreateRenderTargetView(BackBuffer.Get(), &framebufferRTVdesc, &RenderTargetView), "[ FRenderer ]", "Failed to create render target view.", ErrorHandler::EErrorLevel::Critical);
}

void FRenderer::CreateDSV() {
	D3D11_TEXTURE2D_DESC TextureDesc{};
	TextureDesc.Width = WindowInfoReader.Read().ScreenWidth;
	TextureDesc.Height = WindowInfoReader.Read().ScreenHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	ErrorHandler::ReportHRESULT(Device->CreateTexture2D(&TextureDesc, nullptr, DepthStencilBuffer.GetAddressOf()), "[ FRenderer ]", "Failed to create depth stencil buffer.", ErrorHandler::EErrorLevel::Critical);

	D3D11_DEPTH_STENCIL_VIEW_DESC ViewDesc{};
	ViewDesc.Format = TextureDesc.Format;
	ViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	ViewDesc.Texture2D.MipSlice = 0;

	ErrorHandler::ReportHRESULT(Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &ViewDesc, DepthStencilView.GetAddressOf()), "[ FRenderer ]", "Failed to create depth stencil view.", ErrorHandler::EErrorLevel::Critical);
}
