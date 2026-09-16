#include "PCH.h"

#include "FSceneRenderSurface.h"

#include "../ErrorHandler.h"

void FSceneRenderSurface::InitializeSwapChain(ID3D11Device* Device, IDXGISwapChain* InSwapChain) {
	Reset();
	StorageMode = EStorageMode::SwapChain;
	SwapChain = InSwapChain;
	CreateSwapChainResources(Device);
}

void FSceneRenderSurface::InitializeOffscreen(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
	Reset();
	StorageMode = EStorageMode::Offscreen;
	Resize(Device, Width, Height);
}

bool FSceneRenderSurface::Resize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
	if (Device == nullptr || Width == 0 || Height == 0) {
		return false;
	}

	if (StorageMode == EStorageMode::Offscreen && Viewport.Width == static_cast<float>(Width) && Viewport.Height == static_cast<float>(Height)) {
		return true;
	}

	ResetResources();
	if (StorageMode == EStorageMode::SwapChain) {
		DXGI_SWAP_CHAIN_DESC SwapChainDescription{};
		ErrorHandler::ReportHRESULT(SwapChain->GetDesc(&SwapChainDescription), "[ FSceneRenderSurface ]", "Failed to get swap chain description.", ErrorHandler::EErrorLevel::Critical);
		ErrorHandler::ReportHRESULT(SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, SwapChainDescription.Flags), "[ FSceneRenderSurface ]", "Failed to resize swap chain buffers.", ErrorHandler::EErrorLevel::Critical);
		CreateSwapChainResources(Device);
	}
	else if (StorageMode == EStorageMode::Offscreen) {
		CreateOffscreenResources(Device, Width, Height);
	}

	return IsValid();
}

void FSceneRenderSurface::Bind(ID3D11DeviceContext* Context) const {
	if (!IsValid()) {
		return;
	}

	ID3D11RenderTargetView* TargetView = RenderTargetView.Get();
	Context->OMSetRenderTargets(1, &TargetView, DepthStencilView.Get());
	Context->RSSetViewports(1, &Viewport);
}

void FSceneRenderSurface::Clear(ID3D11DeviceContext* Context, const float ClearColor[4]) const {
	if (!IsValid()) {
		return;
	}

	Context->ClearRenderTargetView(RenderTargetView.Get(), ClearColor);
	Context->ClearDepthStencilView(DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FSceneRenderSurface::ClearDepth(ID3D11DeviceContext* Context) const {
	if (!IsValid()) {
		return;
	}

	Context->ClearDepthStencilView(DepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void FSceneRenderSurface::Reset() {
	ResetResources();
	SwapChain.Reset();
	StorageMode = EStorageMode::None;
}

bool FSceneRenderSurface::IsValid() const {
	return RenderTargetView != nullptr && DepthStencilView != nullptr && Viewport.Width > 0.0f && Viewport.Height > 0.0f;
}

const D3D11_VIEWPORT& FSceneRenderSurface::GetViewport() const {
	return Viewport;
}

ID3D11ShaderResourceView* FSceneRenderSurface::GetShaderResourceView() const {
	return ShaderResourceView.Get();
}

void FSceneRenderSurface::CreateSwapChainResources(ID3D11Device* Device) {
	DXGI_SWAP_CHAIN_DESC SwapChainDescription{};
	ErrorHandler::ReportHRESULT(SwapChain->GetDesc(&SwapChainDescription), "[ FSceneRenderSurface ]", "Failed to get swap chain description.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::ReportHRESULT(SwapChain->GetBuffer(0, IID_PPV_ARGS(ColorTexture.GetAddressOf())), "[ FSceneRenderSurface ]", "Failed to get swap chain back buffer.", ErrorHandler::EErrorLevel::Critical);

	D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDescription{};
	RenderTargetViewDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	RenderTargetViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	ErrorHandler::ReportHRESULT(Device->CreateRenderTargetView(ColorTexture.Get(), &RenderTargetViewDescription, RenderTargetView.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create swap chain render target view.", ErrorHandler::EErrorLevel::Critical);

	CreateDepthStencilResources(Device, SwapChainDescription.BufferDesc.Width, SwapChainDescription.BufferDesc.Height);
}

void FSceneRenderSurface::CreateOffscreenResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
	D3D11_TEXTURE2D_DESC TextureDescription{};
	TextureDescription.Width = Width;
	TextureDescription.Height = Height;
	TextureDescription.MipLevels = 1;
	TextureDescription.ArraySize = 1;
	TextureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TextureDescription.SampleDesc.Count = 1;
	TextureDescription.Usage = D3D11_USAGE_DEFAULT;
	TextureDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ErrorHandler::ReportHRESULT(Device->CreateTexture2D(&TextureDescription, nullptr, ColorTexture.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create scene color texture.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::ReportHRESULT(Device->CreateRenderTargetView(ColorTexture.Get(), nullptr, RenderTargetView.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create scene render target view.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::ReportHRESULT(Device->CreateShaderResourceView(ColorTexture.Get(), nullptr, ShaderResourceView.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create scene shader resource view.", ErrorHandler::EErrorLevel::Critical);
	CreateDepthStencilResources(Device, Width, Height);
}

void FSceneRenderSurface::CreateDepthStencilResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
	D3D11_TEXTURE2D_DESC TextureDescription{};
	TextureDescription.Width = Width;
	TextureDescription.Height = Height;
	TextureDescription.MipLevels = 1;
	TextureDescription.ArraySize = 1;
	TextureDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	TextureDescription.SampleDesc.Count = 1;
	TextureDescription.Usage = D3D11_USAGE_DEFAULT;
	TextureDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ErrorHandler::ReportHRESULT(Device->CreateTexture2D(&TextureDescription, nullptr, DepthStencilTexture.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create depth stencil texture.", ErrorHandler::EErrorLevel::Critical);

	D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDescription{};
	DepthStencilViewDescription.Format = TextureDescription.Format;
	DepthStencilViewDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	ErrorHandler::ReportHRESULT(Device->CreateDepthStencilView(DepthStencilTexture.Get(), &DepthStencilViewDescription, DepthStencilView.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create depth stencil view.", ErrorHandler::EErrorLevel::Critical);
	Viewport = { 0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height), 0.0f, 1.0f };
}

void FSceneRenderSurface::ResetResources() {
	DepthStencilView.Reset();
	DepthStencilTexture.Reset();
	ShaderResourceView.Reset();
	RenderTargetView.Reset();
	ColorTexture.Reset();
	Viewport = {};
}
