#include "PCH.h"

#include "EditorViewport.h"

#include <ranges>
#include <utility>

#include "../../FMouseInput.h"

#include "../../Scene/Component/UPrimitiveComponent.h"
#include "../../Scene/AActor.h"

#include "../../Serialize/FEditorConfigManager.h"

#include "../../Scene/UWorld.h"

void EditorViewport::Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, FStateChannel<RenderWindowInfo>::FReader windowReader, FWorldEditorContext& InEditorContext) {
	LineRenderer->Initialize(Device);
	TransformGizmo.Initialize(Device, AssetRegistry, windowReader, InEditorContext);
	WindowInfoReader = windowReader;
	EditorContext = &InEditorContext;
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

	LineRenderer->Render(Context, FLineViewData{
		.ViewProjection = Probe.MainCameraProbe.ViewProjection,
		.ViewportSize = FVector2D{ WindowInfoReader.Read().Viewport.Width, WindowInfoReader.Read().Viewport.Height }
	});

	RenderOrientationAxis(Context, Probe.MainCameraProbe);
}

void EditorViewport::RenderGrid(ELineDepthMode DepthMode) {
	float GridInterval = 1.0f;
	UWorld* World = EditorContext->GetWorld();
	GridInterval = World->GetSettings().GridSize;
	int GridSize = (static_cast<int>(300 / GridInterval));
	float LineLength = static_cast<float>(GridSize) * GridInterval;
	FVector CameraPos = EditorContext->GetCameraState()->Position;

	float SnappedX = std::floor(CameraPos.x / GridInterval) * GridInterval;
	float SnappedY = std::floor(CameraPos.y / GridInterval) * GridInterval;

	for (auto x : std::views::iota(-GridSize, GridSize + 1)) {
		float LineX = SnappedX + static_cast<float>(x) * GridInterval;

		LineRenderer->AddLine(
			FVector3{ LineX, SnappedY - LineLength, 0.f },
			FVector3{ LineX, SnappedY + LineLength, 0.f },
			FVector4{ 0.5f, 0.5f, 0.5f, 1.0f },
			1.0f,
			DepthMode
		);
	}

	for (auto y : std::views::iota(-GridSize, GridSize + 1)) {
		float LineY = SnappedY + static_cast<float>(y) * GridInterval;

		LineRenderer->AddLine(
			FVector3{ SnappedX - LineLength, LineY, 0.f },
			FVector3{ SnappedX + LineLength, LineY, 0.f },
			FVector4{ 0.5f, 0.5f, 0.5f, 1.0f },
			1.0f,
			DepthMode
		);
	}
}

void EditorViewport::RenderAxis(ELineDepthMode DepthMode) {
	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 1.0f, 0.0f, 0.0f }, 1000.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ -1.0f, 0.0f, 0.0f }, 1000.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, DepthMode);

	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 1.0f, 0.0f }, 1000.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, -1.0f, 0.0f }, 1000.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, DepthMode);

	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, 1.0f }, 1000.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, -1.0f }, 1000.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, DepthMode);
}

void EditorViewport::RenderBounds(ELineDepthMode DepthMode)
{
	AActor* Actor = EditorContext->GetSelectedActor();
	if (Actor == nullptr) return;

	USceneComponent* RootComponent = Actor->GetRootComponent();
	if (RootComponent == nullptr || !RootComponent->GetTypeInfo()->IsA(UPrimitiveComponent::StaticTypeInfo())) return;

	const auto* Primitive = static_cast<UPrimitiveComponent*>(RootComponent);
	DirectX::BoundingOrientedBox LocalBounds{};
	LocalBounds = Primitive->GetPickingBox();
	std::array<DirectX::XMFLOAT3, DirectX::BoundingOrientedBox::CORNER_COUNT> Corners{};
	DirectX::BoundingOrientedBox WorldBox;
	LocalBounds.Transform(WorldBox, Primitive->GetComponentToWorld().ToSimpleMath());
	WorldBox.GetCorners(Corners.data());

	const FVector4 LineColor = FVector4{ 1.0f, 1.0f, 0.0f, 1.0f };
	const float Thickness = 3.0f;

	LineRenderer->AddLine(FVector3{ Corners[0] }, FVector3{ Corners[1] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[1] }, FVector3{ Corners[2] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[2] }, FVector3{ Corners[3] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[3] }, FVector3{ Corners[0] }, LineColor, Thickness, DepthMode);

	LineRenderer->AddLine(FVector3{ Corners[4] }, FVector3{ Corners[5] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[5] }, FVector3{ Corners[6] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[6] }, FVector3{ Corners[7] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[7] }, FVector3{ Corners[4] }, LineColor, Thickness, DepthMode);

	LineRenderer->AddLine(FVector3{ Corners[0] }, FVector3{ Corners[4] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[1] }, FVector3{ Corners[5] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[2] }, FVector3{ Corners[6] }, LineColor, Thickness, DepthMode);
	LineRenderer->AddLine(FVector3{ Corners[3] }, FVector3{ Corners[7] }, LineColor, Thickness, DepthMode);
}

void EditorViewport::RenderOrientationAxis(ID3D11DeviceContext* Context, CameraProbe& Probe) {
	FMatrix view = Probe.View;
	view.Translation(FVector3{ 0.0f, 0.0f, 3.0f });

	FMatrix proj = FMatrix::CreateOrthographic(2.5f, 2.5f, 0.1f, 10.f);

	Context->RSSetViewports(1, &OrientationAxisViewport);

	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 1.0f, 0.0f, 0.0f }, 1.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);
	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 1.0f, 0.0f }, 1.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);
	LineRenderer->AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, 1.0f }, 1.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);

	LineRenderer->Render(Context, FLineViewData{
		.ViewProjection = view * proj,
		.ViewportSize = FVector2D{ OrientationAxisViewport.Width, OrientationAxisViewport.Height }
	});
}

void EditorViewport::RenderSceneGuides(ID3D11DeviceContext* Context,FRenderProbe& Probe)
{
	const ELineDepthMode DepthMode = ELineDepthMode::DepthTested;

	RenderGrid(DepthMode);
	RenderAxis(DepthMode);
	RenderBounds(DepthMode);
	LineRenderer->Render(Context,FLineViewData{.ViewProjection = Probe.MainCameraProbe.ViewProjection,
			.ViewportSize = FVector2D{
				WindowInfoReader.Read().Viewport.Width,
				WindowInfoReader.Read().Viewport.Height
			}
		}
	);
}
