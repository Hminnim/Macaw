#pragma once

#include <d3d11.h>

#include "../../Core/Asset/FAssetRegistry.h"
#include "../../Core/Base/FRenderProbe.h"
#include "../../Core/Channel/FStateChannel.h"
#include "../../Scene/FWorldEditorContext.h"
#include "../../FMouseInput.h"
#include "../RenderWindowInfo.h"

#include "ILineRenderer.h"
#include "FLineRenderer.h"
#include "FBatchLineRender.h"
#include "FTransformGizmo.h"

class EditorViewport {
	constexpr static float OrientationAxisSize = 200.0f;

public:
	EditorViewport() = default;
	~EditorViewport() = default;

	EditorViewport(const EditorViewport&) = delete;
	EditorViewport& operator=(const EditorViewport&) = delete;

	EditorViewport(EditorViewport&&) noexcept = default;
	EditorViewport& operator=(EditorViewport&&) noexcept = default;

public:
	void Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, FStateChannel<RenderWindowInfo>::FReader WindowReader, FWorldEditorContext& EditorContext);

	void ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool bMouseCapturedByUI);
	void RenderInProbe(FRenderProbe& Probe);
	void Render(ID3D11DeviceContext* Context, FRenderProbe& Probe);

	void RenderSceneGuides(ID3D11DeviceContext* Context, FRenderProbe& Probe);
	void RenderOrientationAxis(ID3D11DeviceContext* Context, CameraProbe& Probe);

	FStateChannel<uint8>::FReadWriter GetGizmoMode() { return TransformGizmo.GetGizmoMode(); }
	FStateChannel<uint8>::FReadWriter GetGizmoCoordinateSpace() { return TransformGizmo.GetGizmoCoordinateSpace(); }
private:
	void RenderGrid(ELineDepthMode DepthMode);
	void RenderAxis(ELineDepthMode DepthMode);
	void RenderBoundingBox(ELineDepthMode DepthMode);

private:
	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader{};

	std::unique_ptr<ILineRenderer> LineRenderer = std::make_unique<FBatchLineRenderer>();
	FTransformGizmo TransformGizmo{};

	D3D11_VIEWPORT OrientationAxisViewport{ 5.0f, 5.0f, OrientationAxisSize, OrientationAxisSize, 0.0f, 1.0f };
};
