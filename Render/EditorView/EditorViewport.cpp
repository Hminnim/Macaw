#include "PCH.h"

#include "EditorViewport.h"

#include <ranges>
#include <utility>

#include "../../FMouseInput.h"

void EditorViewport::Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, FStateChannel<RenderWindowInfo>::FReader windowReader, FStateChannel<FEditorSelectionState>::FReader selectionReader, FMessageChannel::FSender worldCommandSender) {
	LineRenderer.Initialize(Device);
	TransformGizmo.Initialize(Device, AssetRegistry, windowReader, selectionReader, std::move(worldCommandSender));
	WindowInfoReader = windowReader;
}

void EditorViewport::ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool bMouseCapturedByUI) {
	TransformGizmo.ProcessInput(KeyboardInput, MouseInput, bMouseCapturedByUI);
}

void EditorViewport::RenderInProbe(FRenderProbe& Probe) {
	TransformGizmo.Update(Probe.MainCameraProbe);
	TransformGizmo.Render(Probe);
}

void EditorViewport::Render(ID3D11DeviceContext* Context, FRenderProbe& Probe) {
	ELineDepthMode DepthMode = ELineDepthMode::DepthTested;

	RenderGrid(DepthMode);
	RenderAxis(DepthMode);

	LineRenderer.Render(Context, FLineViewData{
		.ViewProjection = Probe.MainCameraProbe.ViewProjection,
		.ViewportSize = FVector2D{ WindowInfoReader.Read().Viewport.Width, WindowInfoReader.Read().Viewport.Height }
	});

	RenderOrientationAxis(Context, Probe.MainCameraProbe);
}

void EditorViewport::RenderGrid(ELineDepthMode DepthMode) {
	constexpr int GridSize = 50;
	constexpr float LineLength = static_cast<float>(GridSize);

	for (auto x : std::views::iota(-GridSize, GridSize + 1)) {
		if (x == 0) {
			continue;
		}
		LineRenderer.AddLine(FVector3{ static_cast<float>(x), -LineLength, 0.f }, FVector3{ static_cast<float>(x), LineLength, 0.f }, FVector4{ 0.5f, 0.5f, 0.5f, 1.0f }, 1.0f, DepthMode);
	}

	for (auto y : std::views::iota(-GridSize, GridSize + 1)) {
		if (y == 0) {
			continue;
		}
		LineRenderer.AddLine(FVector3{ -LineLength, static_cast<float>(y), 0.f }, FVector3{ LineLength, static_cast<float>(y), 0.f }, FVector4{ 0.5f, 0.5f, 0.5f, 1.0f }, 1.0f, DepthMode);
	}
}

void EditorViewport::RenderAxis(ELineDepthMode DepthMode) {
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 1.0f, 0.0f, 0.0f }, 1000.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ -1.0f, 0.0f, 0.0f }, 1000.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, DepthMode);

	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 1.0f, 0.0f }, 1000.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, -1.0f, 0.0f }, 1000.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, DepthMode);

	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, 1.0f }, 1000.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, -1.0f }, 1000.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, DepthMode);
}

void EditorViewport::RenderOrientationAxis(ID3D11DeviceContext* Context, CameraProbe& Probe) {
	FMatrix view = Probe.View;
	view.Translation(FVector3{ 0.0f, 0.0f, 3.0f });

	FMatrix proj = FMatrix::CreateOrthographic(2.5f, 2.5f, 0.1f, 10.f);

	Context->RSSetViewports(1, &OrientationAxisViewport);

	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 1.0f, 0.0f, 0.0f }, 1.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 1.0f, 0.0f }, 1.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, 1.0f }, 1.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);

	LineRenderer.Render(Context, FLineViewData{
		.ViewProjection = view * proj,
		.ViewportSize = FVector2D{ OrientationAxisViewport.Width, OrientationAxisViewport.Height }
	});
}
