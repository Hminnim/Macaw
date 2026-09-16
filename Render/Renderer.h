#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <memory>

#include "../Core/Base/FRenderProbe.h"
#include "../Core/Asset/FAssetRegistry.h"

#include "Pipeline/UPipeline.h"
#include "../Core/Asset/UMaterial.h"
#include "../Core/Asset/UMesh.h"

#include "../Core/Buffer/TGraphicsArray.h"
#include "../Core/Buffer/TGraphicsRootConstants.h" 

#include "../Core/Channel/FStateChannel.h"
#include "RenderWindowInfo.h"

#include "FTextRenderer.h"
#include "FSceneRenderSurface.h"

#include "../../Scene/FWorldEditorContext.h"

#include "FBillboardRenderer.h"

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

	void BeginSceneRender();
	void BeginUiRender();
	void RenderScene(FRenderProbe& Probe);
	void RenderGizmos(FRenderProbe& Probe);
	void RenderOutline(const TArray<FActorProbe>& ActorProbes, const CameraProbe& MainCameraProbe);
	void RenderText(const FRenderProbe& Probe);
	void RenderActorList(TArray<FActorProbe>& ActorProbes, const CameraProbe& Camera, bool bOutline = false);
	void EndFrame();
	void ResizeSceneSurface(uint32 Width, uint32 Height, float Left, float Top);
	ID3D11ShaderResourceView* GetSceneShaderResourceView() const { return SceneSurface != nullptr ? SceneSurface->GetShaderResourceView() : nullptr; }

	ID3D11Device* GetDevice() const { return Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext.Get(); }

	void BindAssetRegistry(FAssetRegistry* InAssetRegistry) { AssetRegistry = InAssetRegistry; }

	FStateChannel<RenderWindowInfo>::FReader GetWindowInfoReader() const { return WindowInfoChannel.GetReader(); }

	void ReSize(uint32 width, uint32 height);
	
	void Terminate(); 
	void ReportLiveObjects() const;
private:
	void CreateDeviceAndSwapChain(HWND WindowHandle);
	
	void CreateSamplerStates();
	void BindSamplerStates();
	bool UploadLightContext(const FRenderProbe& Probe);

private:
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D11Debug> DebugInterface;
#endif 
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;

	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
	
	std::unique_ptr<IRenderSurface> BackBufferSurface{};
	std::unique_ptr<IRenderSurface> SceneSurface{};

	// s0: LinearWrap, s1: LinearClamp, s2: PointClamp, s3: PointWrap, s4: AnisotropicWrap, s5: ShadowCompare.
	std::array<Microsoft::WRL::ComPtr<ID3D11SamplerState>, 6> SamplerStates{};

	FStateChannel<RenderWindowInfo> WindowInfoChannel{};
	FStateChannel<RenderWindowInfo>::FWriter WindowInfoWriter{ WindowInfoChannel.GetWriter() };
	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader{ WindowInfoChannel.GetReader() };

	FAssetRegistry* AssetRegistry{ nullptr };

	TGraphicsArray<ModelContext, true, true> ModelContextArray{};
	TGraphicsArray<FLightProbe, true, true> LightContextArray{};
	TArray<ModelContext> FrameContexts{};
	TGraphicsRootConstants<64> RootConstants{};

	FTextRenderer TextRenderer{};
	FBillboardRenderer BillboardRenderer{};

	const float ClearColor[4] = { 0.2f, 0.2f, 0.7f, 1.0f };

	//FWorldEditorContext* EditorContext = ;

	size_t RenderIndex = 0;
	uint32 FrameLightCount = 0;
};
