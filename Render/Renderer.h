#pragma once 
#include <d3d11.h>
#include <wrl/client.h>
#include <array>

#include "../Core/Base/FRenderProbe.h"
#include "../Core/Asset/FAssetRegistry.h"

#include "Pipeline/UPipeline.h"
#include "../Core/Asset/UMaterial.h"
#include "../Core/Asset/UMesh.h"

#include "../Core/Buffer/TGraphicsArray.h"
#include "../Core/Buffer/TGraphicsRootConstants.h" 

#include "../Core/Channel/FStateChannel.h"
#include "RenderWindowInfo.h"

class FRenderer {
	struct ModelContext {
		FMatrix World{};
		uint32 MaterialIndex{ UINT32_MAX };
		uint32 Flags{ 0x0000'0000 };
	};

public:
	FRenderer() = default;
	~FRenderer();
	
	FRenderer(const FRenderer&) = delete;
	FRenderer& operator=(const FRenderer&) = delete;

	FRenderer(FRenderer&&) = delete;
	FRenderer& operator=(FRenderer&&) = delete;

public:
	void Create(HWND WindowHandle, UINT width, UINT height);

	void BeginFrame();
	void RenderScene(FRenderProbe& Probe);
	void RenderGizmos(FRenderProbe& Probe);
	void RenderActorList(TArray<FActorProbe>& ActorProbes, const CameraProbe& Camera);
	void EndFrame();

	ID3D11Device* GetDevice() const { return Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext.Get(); }

	void BindAssetRegistry(FAssetRegistry* InAssetRegistry) { AssetRegistry = InAssetRegistry; }

	FStateChannel<RenderWindowInfo>::FReader GetWindowInfoReader() const { return WindowInfoChannel.GetReader(); }

	void ReSize(uint32 width, uint32 height);
private:
	void CreateDeviceAndSwapChain(HWND WindowHandle);
	
	void CreateRTV();
	void CreateDSV();
	void CreateSamplerStates();

private:
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;

	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
	
	Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
	
	Microsoft::WRL::ComPtr<ID3D11Texture2D> DepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView;

	// s0: LinearWrap, s1: LinearClamp, s2: PointClamp, s3: PointWrap, s4: AnisotropicWrap, s5: ShadowCompare.
	std::array<Microsoft::WRL::ComPtr<ID3D11SamplerState>, 6> SamplerStates{};

	FStateChannel<RenderWindowInfo> WindowInfoChannel{};
	FStateChannel<RenderWindowInfo>::FWriter WindowInfoWriter{ WindowInfoChannel.GetWriter() };
	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader{ WindowInfoChannel.GetReader() };

	FAssetRegistry* AssetRegistry{ nullptr };

	TGraphicsArray<ModelContext> ModelContextArray{};
	TGraphicsRootConstants<64> RootConstants{};

	const float ClearColor[4] = { 0.2f, 0.2f, 0.7f, 1.0f };
};
