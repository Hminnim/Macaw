#include "PCH.h"

#include "FTransformGizmo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <DirectXCollision.h>
#include <limits>

#include "../../Core/Asset/BasicGeometry/Corn.h"
#include "../../Core/Asset/BasicGeometry/Cylinder.h"
#include "../../Core/Asset/UColorMaterial.h"

void FTransformGizmo::Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, FStateChannel<RenderWindowInfo>::FReader InWindowInfoReader, FStateChannel<FEditorSelectionState>::FReader InSelectionReader, FMessageChannel::FSender InWorldCommandSender) {

	CylinderMesh = AssetRegistry.EmplaceAsset<UMesh>(Device, "CylinderMesh", "./Content/Metadata/CylinderMesh.meta");
	ConeMesh = AssetRegistry.EmplaceAsset<UMesh>(Device, "ConeMesh", "./Content/Metadata/ConeMesh.meta");

	RedMaterial = AssetRegistry.EmplaceAsset<UColorMaterial>(Device, "Red", "./Content/Metadata/RedMaterial.meta");
	GreenMaterial = AssetRegistry.EmplaceAsset<UColorMaterial>(Device, "Green", "./Content/Metadata/GreenMaterial.meta");
	BlueMaterial = AssetRegistry.EmplaceAsset<UColorMaterial>(Device, "Blue", "./Content/Metadata/BlueMaterial.meta");

	GizmoPipeline = AssetRegistry.EmplaceAsset<UPipeline>(Device, "GizmoPipeline", "./Content/Metadata/GizmoPipeline.meta");

	WindowInfoReader = InWindowInfoReader;
	SelectionReader = InSelectionReader;
	WorldCommandSender.emplace(std::move(InWorldCommandSender));

	GizmoMode = GizmoModeChannel.GetReadWriter();
	GizmoMode.Emplace(static_cast<uint8>(EModifyMode::Translate));
}

void FTransformGizmo::ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool bMouseCapturedByUI) {
	if (KeyboardInput.GetKeyState('T') == EKeyState::Pressed) {
 		GizmoMode.Emplace(static_cast<uint8>(EModifyMode::Translate));
	}

	if (KeyboardInput.GetKeyState('R') == EKeyState::Pressed) {
		GizmoMode.Emplace(static_cast<uint8>(EModifyMode::Rotate));
	}

	if (KeyboardInput.GetKeyState('Y') == EKeyState::Pressed) {
		GizmoMode.Emplace(static_cast<uint8>(EModifyMode::Scale));
	}

	const EKeyState LeftState = MouseInput.GetKeyState(Left);
	const FMouseInput::DragCapture& Capture = MouseInput.GetDragCapture(Left);

	if (DragSession.has_value()) {
		if (LeftState == EKeyState::Down) {
			if (const std::optional<FRay> Ray = MakeWorldRay(Capture.current)) {
				UpdateDrag(*Ray);
			}
		} else if (LeftState == EKeyState::Released) {
			if (const std::optional<FRay> Ray = MakeWorldRay(Capture.current)) {
				UpdateDrag(*Ray);
			}
			EndDrag(false);
		}


		return;
	}

	if (bMouseCapturedByUI || LeftState != EKeyState::Pressed || !bVisible) {
		return;
	}

	const std::optional<FRay> Ray = MakeWorldRay(Capture.start);
	if (!Ray.has_value()) {
		return;
	}

	const std::optional<FAxisHit> Hit = HitTest(*Ray);
	if (!Hit.has_value() || !BeginDrag(Hit->Axis, *Ray)) {
		return;
	}

	MouseInput.Consume(Left);

	if (Capture.current.x != Capture.start.x || Capture.current.y != Capture.start.y) {
		if (const std::optional<FRay> CurrentRay = MakeWorldRay(Capture.current)) {
			UpdateDrag(*CurrentRay);
		}
	}
}

void FTransformGizmo::Update(const CameraProbe& Camera) {
	LastCamera = Camera;
	bHasCamera = true;

	if (!SelectionReader.HasValue() || !WindowInfoReader.HasValue()) {
		if (DragSession.has_value()) {
			EndDrag(true);
		}
		bVisible = false;
		return;
	}

	const FEditorSelectionState& Selection = SelectionReader.Read();
	if (!Selection.TransformTargetHandle.IsValid()) {
		bVisible = false;
		return;
	}

	if (DragSession.has_value() && DragSession->TargetHandle != Selection.TransformTargetHandle) {
		EndDrag(true);
	}

	CurrentSelection = Selection;

	FVector3 TargetScale{};
	FQuat TargetRotation{};
	FVector3 TargetTranslation{};
	FMatrix TargetWorld = Selection.TargetWorld;
	if (!TargetWorld.Decompose(TargetScale, TargetRotation, TargetTranslation)) {
		bVisible = false;
		return;
	}

	GizmoWorldTransform = FMatrix::CreateFromQuaternion(TargetRotation) * FMatrix::CreateTranslation(TargetTranslation);
	// 월드축 기준 GizmoWorldTransform = FMatrix::CreateTranslation(TargetTranslation);

	FVector3 BoundsExtent{};
	UpdateBoundsInGizmoSpace(Selection, BoundsCenterInGizmoSpace, BoundsExtent);

	const RenderWindowInfo& WindowInfo = WindowInfoReader.Read();
	const float ViewportHeight = WindowInfo.Viewport.Height;
	const float ProjectionYScale = Camera.Projection.m[1][1];
	const FVector3 BoundsCenterWorld = FVector3::Transform(BoundsCenterInGizmoSpace, GizmoWorldTransform);
	const float ViewDepth = FVector3::Transform(BoundsCenterWorld, Camera.View).z;

	if (ViewportHeight <= 0.0f || std::abs(ProjectionYScale) <= std::numeric_limits<float>::epsilon() || ViewDepth <= 0.0f) {
		bVisible = false;
		return;
	}

	const float WorldUnitsPerPixel = (2.0f * ViewDepth) / (ViewportHeight * ProjectionYScale);
	CurrentWorkUnitsPerPixel = WorldUnitsPerPixel;

	SetArrow(BoundsCenterInGizmoSpace, WorldUnitsPerPixel);
	bVisible = true;
}

void FTransformGizmo::SetArrow(const FVector3& Pivot, float WorldUnitsPerPixel) {

	const float ShaftLength = ShaftLengthPixels * WorldUnitsPerPixel;
	const float ConeLength = ConeLengthPixels * WorldUnitsPerPixel;
	const float ShaftRadius = ShaftRadiusPixels * WorldUnitsPerPixel;
	const float ConeRadius = ConeRadiusPixels * WorldUnitsPerPixel;
	const float PickRadius = PickRadiusPixels * WorldUnitsPerPixel;
	const float BoundsGap = BoundsGapPixels * WorldUnitsPerPixel;

	const float HalfShaftLength = ShaftLength * 0.5f;
	const float HalfConeLength = ConeLength * 0.5f;
	const float TotalLength = ShaftLength + ConeLength;

	const float StartX = Pivot.x + BoundsGap;
	const float StartY = Pivot.y + BoundsGap;
	const float StartZ = Pivot.z + BoundsGap;

	CylinderXAxisTransform = FMatrix::CreateScale(ShaftRadius, ShaftLength, ShaftRadius) * FMatrix::CreateRotationZ(DirectX::XMConvertToRadians(-90.0f)) * FMatrix::CreateTranslation(StartX + HalfShaftLength, Pivot.y, Pivot.z);

	CylinderYAxisTransform = FMatrix::CreateScale(ShaftRadius, ShaftLength, ShaftRadius) * FMatrix::CreateTranslation(Pivot.x, StartY + HalfShaftLength, Pivot.z);

	CylinderZAxisTransform = FMatrix::CreateScale(ShaftRadius, ShaftLength, ShaftRadius) * FMatrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f)) * FMatrix::CreateTranslation(Pivot.x, Pivot.y, StartZ + HalfShaftLength);

	ConeXAxisTransform = FMatrix::CreateScale(ConeRadius, ConeLength, ConeRadius) * FMatrix::CreateRotationZ(DirectX::XMConvertToRadians(-90.0f)) * FMatrix::CreateTranslation(StartX + ShaftLength + HalfConeLength, Pivot.y, Pivot.z);

	ConeYAxisTransform = FMatrix::CreateScale(ConeRadius, ConeLength, ConeRadius) * FMatrix::CreateTranslation(Pivot.x, StartY + ShaftLength + HalfConeLength, Pivot.z);

	ConeZAxisTransform = FMatrix::CreateScale(ConeRadius, ConeLength, ConeRadius) * FMatrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f)) * FMatrix::CreateTranslation(Pivot.x, Pivot.y, StartZ + ShaftLength + HalfConeLength);

	AxisHitProxies = {
		FAxisHitProxy{
			.Axis = EAxis::X,
			.Center = FVector3{ StartX + TotalLength * 0.5f, Pivot.y, Pivot.z },
			.Extent = FVector3{ TotalLength * 0.5f, PickRadius, PickRadius }
		},
		FAxisHitProxy{
			.Axis = EAxis::Y,
			.Center = FVector3{ Pivot.x, StartY + TotalLength * 0.5f, Pivot.z },
			.Extent = FVector3{ PickRadius, TotalLength * 0.5f, PickRadius }
		},
		FAxisHitProxy{
			.Axis = EAxis::Z,
			.Center = FVector3{ Pivot.x, Pivot.y, StartZ + TotalLength * 0.5f },
			.Extent = FVector3{ PickRadius, PickRadius, TotalLength * 0.5f }
		}
	};
}

void FTransformGizmo::UpdateBoundsInGizmoSpace(const FEditorSelectionState& Selection, FVector3& OutCenter, FVector3& OutExtent) const {
	DirectX::BoundingOrientedBox LocalBounds{};
	LocalBounds.Center = Selection.BoundsCenter.ToSimpleMath();
	LocalBounds.Extents = Selection.BoundsExtent.ToSimpleMath();
	LocalBounds.Orientation = Selection.BoundsOrientation;

	std::array<DirectX::XMFLOAT3, DirectX::BoundingOrientedBox::CORNER_COUNT> Corners{};
	LocalBounds.GetCorners(Corners.data());

	const FMatrix ColliderToGizmo = Selection.ColliderWorld * GizmoWorldTransform.Invert();
	FVector3 Minimum{
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max()
	};
	FVector3 Maximum{
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest()
	};

	for (const DirectX::XMFLOAT3& Corner : Corners) {
		const FVector3 PointInGizmoSpace = FVector3::Transform(FVector3{ Corner }, ColliderToGizmo);
		Minimum = FVector3::Min(Minimum, PointInGizmoSpace);
		Maximum = FVector3::Max(Maximum, PointInGizmoSpace);
	}

	OutCenter = (Minimum + Maximum) * 0.5f;
	OutExtent = (Maximum - Minimum) * 0.5f;
}

std::optional<FRay> FTransformGizmo::MakeWorldRay(const POINT& ScreenPosition) const {
	if (!bHasCamera || !WindowInfoReader.HasValue()) {
		return std::nullopt;
	}

	const D3D11_VIEWPORT& Viewport = WindowInfoReader.Peek().Viewport;
	if (Viewport.Width <= 0.0f || Viewport.Height <= 0.0f) {
		return std::nullopt;
	}

	const float ViewportX = static_cast<float>(ScreenPosition.x) - Viewport.TopLeftX;
	const float ViewportY = static_cast<float>(ScreenPosition.y) - Viewport.TopLeftY;
	const float NdcX = 2.0f * ViewportX / Viewport.Width - 1.0f;
	const float NdcY = 1.0f - 2.0f * ViewportY / Viewport.Height;

	const FMatrix InverseViewProjection = LastCamera.ViewProjection.Invert();
	const FVector3 RayOrigin = FVector3::Transform(FVector3{ NdcX, NdcY, 0.0f }, InverseViewProjection);
	FVector3 RayDirection = FVector3::Transform(FVector3{ NdcX, NdcY, 1.0f }, InverseViewProjection) - RayOrigin;

	if (RayDirection.LengthSquared() <= std::numeric_limits<float>::epsilon()) {
		return std::nullopt;
	}

	RayDirection.Normalize();
	return FRay{ RayOrigin.ToSimpleMath(), RayDirection.ToSimpleMath() };
}

std::optional<FTransformGizmo::FAxisHit> FTransformGizmo::HitTest(const FRay& WorldRay) const {
	const FMatrix InverseGizmoWorld = GizmoWorldTransform.Invert();
	const FVector3 LocalOrigin = FVector3::Transform(FVector3(WorldRay.position), InverseGizmoWorld);
	FVector3 LocalDirection = FVector3::TransformNormal(FVector3(WorldRay.direction), InverseGizmoWorld);
	if (LocalDirection.LengthSquared() <= std::numeric_limits<float>::epsilon()) {
		return std::nullopt;
	}
	LocalDirection.Normalize();

	const FRay LocalRay{ LocalOrigin.ToSimpleMath(), LocalDirection.ToSimpleMath() };
	std::optional<FAxisHit> NearestHit;

	for (const FAxisHitProxy& Proxy : AxisHitProxies) {
		const DirectX::BoundingBox Box{ Proxy.Center.ToSimpleMath(), Proxy.Extent.ToSimpleMath() };
		float Distance = 0.0f;
		if (Box.Intersects(LocalRay.position, LocalRay.direction, Distance) && (!NearestHit.has_value() || Distance < NearestHit->Distance)) {
			NearestHit = FAxisHit{
				.Axis = Proxy.Axis,
				.Distance = Distance
			};
		}
	}

	return NearestHit;
}

bool FTransformGizmo::BeginDrag(EAxis Axis, const FRay& WorldRay) {
	if (!WorldCommandSender.has_value() || Axis == EAxis::None) {
		return false;
	}

	FVector3 AxisWorld = GetWorldAxis(Axis);
	if (AxisWorld.LengthSquared() <= std::numeric_limits<float>::epsilon()) {
		return false;
	}
	AxisWorld.Normalize();

	const FVector3 InteractionPivotWorld = FVector3::Transform(BoundsCenterInGizmoSpace, GizmoWorldTransform);
	const FVector3 CameraPosition = LastCamera.View.Invert().Translation();
	FVector3 ViewDirection = InteractionPivotWorld - CameraPosition;
	if (ViewDirection.LengthSquared() <= std::numeric_limits<float>::epsilon()) {
		return false;
	}
	ViewDirection.Normalize();

	FVector3 PlaneNormal = ViewDirection - AxisWorld * ViewDirection.Dot(AxisWorld);
	if (PlaneNormal.LengthSquared() <= 0.000001f) {
		const FVector3 Fallback = std::abs(AxisWorld.Dot(FVector3::UnitY)) < 0.95f ? FVector3::UnitY : FVector3::UnitX;
		PlaneNormal = Fallback - AxisWorld * Fallback.Dot(AxisWorld);
	}
	PlaneNormal.Normalize();

	FDragSession NewSession{
		.SessionId = AcquireTransformEditSessionId(),
		.TargetHandle = CurrentSelection.TransformTargetHandle,
		.InitialWorld = CurrentSelection.TargetWorld,
		.AxisWorld = AxisWorld,
		.InteractionPivotWorld = InteractionPivotWorld,
		.DragPlaneNormal = PlaneNormal,
		.InitialAxisParameter = 0.0f,
		.InitialTransformRevision = CurrentSelection.TransformRevision,
		.DragAxis = Axis,	
		.WorkUnitsPerPixel = CurrentWorkUnitsPerPixel
	};

	if (!GetAxisParameterOnDragPlane(WorldRay, NewSession, NewSession.InitialAxisParameter)) {
		return false;
	}

	DragSession = NewSession;
	SendTransformEdit(NewSession.SessionId, ETransformEditPhase::Begin, NewSession.TargetHandle, NewSession.InitialWorld, NewSession.InitialTransformRevision);
	return true;
}

void FTransformGizmo::UpdateDrag(const FRay& WorldRay) {

	if (!DragSession.has_value()) {
			return;
	}
	
	auto& Session = *DragSession;


	float CurrentAxisParameter = 0.0f;
	if (!GetAxisParameterOnDragPlane(WorldRay, *DragSession, CurrentAxisParameter)) {
		return;
	}
	
	const float Delta = CurrentAxisParameter - DragSession->InitialAxisParameter;
	FMatrix DesiredWorld = DragSession->InitialWorld;

	const EModifyMode CurrentModifyMode = GizmoMode.HasValue() ? static_cast<EModifyMode>(GizmoMode.Peek()) : EModifyMode::None;

	if (CurrentModifyMode == EModifyMode::Translate) {
		DesiredWorld.Translation(DragSession->InitialWorld.Translation() + DragSession->AxisWorld * Delta);
	}
	else if (CurrentModifyMode == EModifyMode::Rotate) {
		const float RotationSpeed = std::max(120.f * Session.WorkUnitsPerPixel, 0.0001f);
		const float AngleDelta = Delta / RotationSpeed;
		const FMatrix RotationMatrix = FMatrix::CreateFromQuaternion(FQuat::CreateFromAxisAngle(DragSession->AxisWorld.ToSimpleMath(), AngleDelta));
		const FVector3 Pivot = DragSession->InteractionPivotWorld;

		DesiredWorld = Session.InitialWorld * FMatrix::CreateTranslation(-Pivot) * RotationMatrix * FMatrix::CreateTranslation(Pivot);
	}
	else if (CurrentModifyMode == EModifyMode::Scale) {
		const float ScaleSpeed = std::max(100.f * Session.WorkUnitsPerPixel, 0.0001f);
		const float ScaleFactor = std::max(0.01f, 1.f + Delta / ScaleSpeed);

		DesiredWorld = FMatrix::CreateScale(FVector3(ScaleFactor, ScaleFactor, ScaleFactor)) * Session.InitialWorld;

		FVector3 InitialScale{};
		FQuat InitialRotation{}; 
		FVector3 InitialTranslation{};

		Session.InitialWorld.Decompose(InitialScale, InitialRotation, InitialTranslation);

		switch (Session.DragAxis) {
		case EAxis::X:
			InitialScale.x *= ScaleFactor; 
			break;
		case EAxis::Y:
			InitialScale.y *= ScaleFactor; 
			break;
		case EAxis::Z:
			InitialScale.z *= ScaleFactor; 
			break;
		default:
			return;
		}

		DesiredWorld = FMatrix::CreateScale(InitialScale) * FMatrix::CreateFromQuaternion(InitialRotation) * FMatrix::CreateTranslation(InitialTranslation);

	}

	SendTransformEdit(DragSession->SessionId, ETransformEditPhase::Update, DragSession->TargetHandle, DesiredWorld, DragSession->InitialTransformRevision);
}

void FTransformGizmo::EndDrag(bool bCancel) {
	if (!DragSession.has_value()) {
		return;
	}

	SendTransformEdit(DragSession->SessionId, bCancel ? ETransformEditPhase::Cancel : ETransformEditPhase::Commit, DragSession->TargetHandle, DragSession->InitialWorld, DragSession->InitialTransformRevision);
	DragSession.reset();
}

bool FTransformGizmo::GetAxisParameterOnDragPlane(const FRay& WorldRay, const FDragSession& Session, float& OutParameter) const {
	const FPlane DragPlane{ Session.InteractionPivotWorld.ToSimpleMath(), Session.DragPlaneNormal.ToSimpleMath() };
	float Distance = 0.0f;
	if (!WorldRay.Intersects(DragPlane, Distance)) {
		return false;
	}

	const FVector3 HitPosition(WorldRay.position + WorldRay.direction * Distance);
	OutParameter = (HitPosition - Session.InteractionPivotWorld).Dot(Session.AxisWorld);
	return true;
}

FVector3 FTransformGizmo::GetWorldAxis(EAxis Axis) const {
	switch (Axis) {
	case EAxis::X:
		return GizmoWorldTransform.Right();
	case EAxis::Y:
		return GizmoWorldTransform.Up();
	case EAxis::Z:
		return GizmoWorldTransform.Forward();
	default:
		return FVector3::Zero;
	}
}

void FTransformGizmo::SendTransformEdit(std::uint64_t SessionId, ETransformEditPhase Phase, FObjectHandle TargetHandle, const FMatrix& DesiredWorld, std::uint64_t ExpectedTransformRevision) {
	if (!WorldCommandSender.has_value()) {
		return;
	}

	WorldCommandSender->TryEmplace<FTransformEditRequestMessage>(SessionId, Phase, TargetHandle, DesiredWorld, ExpectedTransformRevision);
}

void FTransformGizmo::Render(FRenderProbe& Probe) {
	if (!bVisible) {
		return;
	}

	const FMatrix CylinderXWorld = CylinderXAxisTransform * GizmoWorldTransform;
	const FMatrix CylinderYWorld = CylinderYAxisTransform * GizmoWorldTransform;
	const FMatrix CylinderZWorld = CylinderZAxisTransform * GizmoWorldTransform;

	Probe.GizmoProbes.emplace_back(FActorProbe{
		.World = CylinderXWorld,
		.MeshHandle = CylinderMesh,
		.MaterialHandle = RedMaterial,
		.PipelineHandle = GizmoPipeline
	});

	Probe.GizmoProbes.emplace_back(FActorProbe{
		.World = CylinderYWorld,
		.MeshHandle = CylinderMesh,
		.MaterialHandle = GreenMaterial,
		.PipelineHandle = GizmoPipeline
	});

	Probe.GizmoProbes.emplace_back(FActorProbe{
		.World = CylinderZWorld,
		.MeshHandle = CylinderMesh,
		.MaterialHandle = BlueMaterial,
		.PipelineHandle = GizmoPipeline
	});

	const FMatrix ConeXWorld = ConeXAxisTransform * GizmoWorldTransform;
	const FMatrix ConeYWorld = ConeYAxisTransform * GizmoWorldTransform;
	const FMatrix ConeZWorld = ConeZAxisTransform * GizmoWorldTransform;

	Probe.GizmoProbes.emplace_back(FActorProbe{
		.World = ConeXWorld,
		.MeshHandle = ConeMesh,
		.MaterialHandle = RedMaterial,
		.PipelineHandle = GizmoPipeline
	});

	Probe.GizmoProbes.emplace_back(FActorProbe{
		.World = ConeYWorld,
		.MeshHandle = ConeMesh,
		.MaterialHandle = GreenMaterial,
		.PipelineHandle = GizmoPipeline
	});

	Probe.GizmoProbes.emplace_back(FActorProbe{
		.World = ConeZWorld,
		.MeshHandle = ConeMesh,
		.MaterialHandle = BlueMaterial,
		.PipelineHandle = GizmoPipeline
	});
}
