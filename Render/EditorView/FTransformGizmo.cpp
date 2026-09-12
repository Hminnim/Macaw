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
	CubeMesh = AssetRegistry.EmplaceAsset<UMesh>(Device,"CubeMesh","./Content/Metadata/CubeMesh.meta");
	GizmoTorusMesh = AssetRegistry.EmplaceAsset<UMesh>(Device, "GizmoTorusMesh", "./Content/Metadata/GizmoTorusMesh.meta");

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

	const EModifyMode CurrentMode =GizmoMode.HasValue() ? static_cast<EModifyMode>(GizmoMode.Peek()) : EModifyMode::None;

	switch (CurrentMode) {
	case EModifyMode::Translate:
		SetTranslate(BoundsCenterInGizmoSpace, WorldUnitsPerPixel);
		break;

	case EModifyMode::Scale:
		SetScale(BoundsCenterInGizmoSpace, WorldUnitsPerPixel);
		break;

	case EModifyMode::Rotate:
		SetRotate(BoundsCenterInGizmoSpace, WorldUnitsPerPixel);
		break;

	default:
		bVisible = false;
		return;
	}

	bVisible = true;
}

void FTransformGizmo::SetTranslate(const FVector3& Pivot, float WorldUnitsPerPixel) {

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

void FTransformGizmo::SetRotate(const FVector3& Pivot, float WorldUnitsPerPixel) {

	constexpr float RingOuterRadiusPixels = 76.0f;
	constexpr float RingPickThicknessPixels = 8.0f;

	const float RingOuterRadius = RingOuterRadiusPixels * WorldUnitsPerPixel;
	const float RingPickThickness = RingPickThicknessPixels * WorldUnitsPerPixel;

	/*
	 * 기본 Torus의 바깥 반지름은 0.5이므로,
	 * 목표 바깥 반지름을 만들기 위해 2배로 스케일한다.
	 */

	CurrentRingRadius = RingOuterRadius * (0.50f / 0.49f);
	CurrentRingPickHalfWidth = RingPickThicknessPixels * WorldUnitsPerPixel;

	const float TorusScale = RingOuterRadius * 2.0f;

	TorusXAxisTransform =FMatrix::CreateScale(TorusScale,TorusScale,TorusScale)
		* FMatrix::CreateRotationZ(DirectX::XMConvertToRadians(-90.0f))
		* FMatrix::CreateTranslation(Pivot);

	TorusYAxisTransform =FMatrix::CreateScale(TorusScale,TorusScale,TorusScale)
		* FMatrix::CreateTranslation(Pivot);

	TorusZAxisTransform =FMatrix::CreateScale(TorusScale,TorusScale,TorusScale)
		* FMatrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f))
		* FMatrix::CreateTranslation(Pivot);

}

void FTransformGizmo::SetScale(const FVector3& Pivot, float WorldUnitsPerPixel) {

	constexpr float ScaleBoxSizePixels = 18.0f;

	const float ShaftLength = ShaftLengthPixels * WorldUnitsPerPixel;
	const float ShaftRadius = ShaftRadiusPixels * WorldUnitsPerPixel;
	const float BoxSize = ScaleBoxSizePixels * WorldUnitsPerPixel;
	const float PickRadius = PickRadiusPixels * WorldUnitsPerPixel;
	const float BoundsGap = BoundsGapPixels * WorldUnitsPerPixel;

	const float HalfShaftLength = ShaftLength * 0.5f;
	const float HalfBoxSize = BoxSize * 0.5f;
	const float TotalLength = ShaftLength + BoxSize;

	const float StartX = Pivot.x + BoundsGap;
	const float StartY = Pivot.y + BoundsGap;
	const float StartZ = Pivot.z + BoundsGap;

	CylinderXAxisTransform = FMatrix::CreateScale(ShaftRadius,ShaftLength,ShaftRadius)
		* FMatrix::CreateRotationZ(DirectX::XMConvertToRadians(-90.0f))
		* FMatrix::CreateTranslation(StartX + HalfShaftLength,Pivot.y,Pivot.z);

	CylinderYAxisTransform = FMatrix::CreateScale(ShaftRadius,ShaftLength,ShaftRadius)
		* FMatrix::CreateTranslation(Pivot.x, StartY + HalfShaftLength, Pivot.z);

	CylinderZAxisTransform = FMatrix::CreateScale(ShaftRadius, ShaftLength,ShaftRadius)
		* FMatrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f))
		* FMatrix::CreateTranslation(Pivot.x,Pivot.y,StartZ + HalfShaftLength);

	CubeXAxisTransform = FMatrix::CreateScale(BoxSize, BoxSize, BoxSize)
		* FMatrix::CreateTranslation(StartX + ShaftLength + HalfBoxSize,Pivot.y,Pivot.z);

	CubeYAxisTransform =FMatrix::CreateScale(BoxSize, BoxSize, BoxSize)
		* FMatrix::CreateTranslation(Pivot.x,StartY + ShaftLength + HalfBoxSize,Pivot.z);

	CubeZAxisTransform =FMatrix::CreateScale(BoxSize, BoxSize, BoxSize)
		* FMatrix::CreateTranslation(Pivot.x,Pivot.y,StartZ + ShaftLength + HalfBoxSize);

	AxisHitProxies = {FAxisHitProxy{
			.Axis = EAxis::X,
			.Center = FVector3{StartX + TotalLength * 0.5f,Pivot.y,Pivot.z},
			.Extent = FVector3{TotalLength * 0.5f,PickRadius,PickRadius}
		},
		FAxisHitProxy{
			.Axis = EAxis::Y,
			.Center = FVector3{Pivot.x,StartY + TotalLength * 0.5f,Pivot.z},
			.Extent = FVector3{PickRadius,TotalLength * 0.5f,PickRadius}
		},
		FAxisHitProxy{
			.Axis = EAxis::Z,
			.Center = FVector3{Pivot.x,Pivot.y,StartZ + TotalLength * 0.5f},
			.Extent = FVector3{PickRadius,PickRadius,TotalLength * 0.5f}
		}
	};
}

void FTransformGizmo::UpdateBoundsInGizmoSpace(const FEditorSelectionState& Selection, FVector3& OutCenter, FVector3& OutExtent) const {
	DirectX::BoundingOrientedBox LocalBounds{};
	LocalBounds.Center = Selection.BoundsCenter.ToSimpleMath();
	LocalBounds.Extents = Selection.BoundsExtent.ToSimpleMath();
	LocalBounds.Orientation.x = Selection.BoundsOrientation.x;
	LocalBounds.Orientation.y = Selection.BoundsOrientation.y;
	LocalBounds.Orientation.z = Selection.BoundsOrientation.z;
	LocalBounds.Orientation.w = Selection.BoundsOrientation.w;

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
	const EModifyMode CurrentMode = GizmoMode.HasValue() ? static_cast<EModifyMode>(GizmoMode.Peek()) : EModifyMode::None;

	if (CurrentMode == EModifyMode::Rotate)
	{
		const EAxis Axis[3]{EAxis::X,EAxis::Y,EAxis::Z};
		const FVector3 PlaneNormals[3]{FVector3::UnitX,FVector3::UnitY,FVector3::UnitZ};
		for (int i = 0; i < 3; i++)
		{
			FVector3 PlaneNormal = PlaneNormals[i];
			float Denominator = LocalDirection.Dot(PlaneNormal);
			if (std::abs(Denominator) <= 0.000001f) // 레이와 평면이 거의 평행한 경우 패스
			{
				continue;
			}
			float Distance = (BoundsCenterInGizmoSpace - LocalOrigin).Dot(PlaneNormal) / Denominator;
			if (Distance < 0.0f) // 교차점이 카메라 밖에 있는 경우
			{
				continue;
			}
			FVector3 HitPosition = LocalOrigin + LocalDirection * Distance;
			float DistanceFromPivot = (HitPosition - BoundsCenterInGizmoSpace).Length(); // 중심과 마우스를 클릭한 사이의 거리
			float DistanceFromRadius = std::abs(DistanceFromPivot - CurrentRingRadius); // 그 거리 - 현재 링 반지름 => 해당값이 허용 오차 사이에 있어야 인정
			if (DistanceFromRadius <= CurrentRingPickHalfWidth) // CurrentRingPickHalfWidth = 허용 오차
			{
				if (!NearestHit.has_value() || Distance < NearestHit->Distance) // t가 가장 작은걸 선택
				{
					NearestHit = FAxisHit{
						.Axis = Axis[i],
						.Distance = Distance
					};
				}
			}
		}
		return NearestHit;
	}

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

	// 메시지를 보낼 수 없거나 유효한 축이 아니면 드래그를 시작하지 않는다.
	if (!WorldCommandSender.has_value() || Axis == EAxis::None) 
	{
		return false;
	}

	//드래그를 시작한 순간의 모드를 고정한다.	
	const EModifyMode CurrentMode = GizmoMode.HasValue() ? static_cast<EModifyMode>(GizmoMode.Peek()) : EModifyMode::None;

	if (CurrentMode == EModifyMode::None) 
	{
		return false;
	}

	// 선택한 기즈모 축을 월드 공간 방향으로 변환한다.
	FVector3 AxisWorld = GetWorldAxis(Axis);

	if (AxisWorld.LengthSquared() <= std::numeric_limits<float>::epsilon()) 
	{
		return false;
	}

	AxisWorld.Normalize();

	// 기즈모가 표시된 위치를 월드 공간 Pivot으로 변환한다.
	const FVector3 InteractionPivotWorld = FVector3::Transform(BoundsCenterInGizmoSpace,GizmoWorldTransform);

	// 우선 모든 모드에서 공통으로 사용하는 세션 정보를 저장한다.
	FDragSession NewSession{};

	NewSession.SessionId = AcquireTransformEditSessionId();
	NewSession.TargetHandle = CurrentSelection.TransformTargetHandle;
	NewSession.InitialWorld = CurrentSelection.TargetWorld;
	NewSession.InitialTransformRevision = CurrentSelection.TransformRevision;
	NewSession.ModifyMode = CurrentMode;
	NewSession.DragAxis = Axis;
	NewSession.AxisWorld = AxisWorld;
	NewSession.InteractionPivotWorld = InteractionPivotWorld;
	NewSession.WorkUnitsPerPixel = CurrentWorkUnitsPerPixel;

	// Rotation은 링 평면을 사용한다.
	if (CurrentMode == EModifyMode::Rotate) {
		// 회전 링 평면은 회전축에 수직이므로 평면 법선은 회전축과 같다.
		NewSession.DragPlaneNormal = AxisWorld;

		const FPlane RotationPlane
		{
			InteractionPivotWorld.ToSimpleMath(), 
			AxisWorld.ToSimpleMath()
		};

		float Distance = 0.0f;

		if (!WorldRay.Intersects(RotationPlane,Distance)|| Distance < 0.0f) 
		{
			return false;
		}

		const FVector3 HitPosition
		{
			WorldRay.position+ WorldRay.direction * Distance
		};

		//Pivot에서 클릭점으로 향하는 방향이 회전 시작 방향이다.
		FVector3 InitialDirection = HitPosition - InteractionPivotWorld;

		// 부동소수점 오차로 남을 수 있는 회전축 방향 성분을 제거한다.
		InitialDirection = InitialDirection - AxisWorld * InitialDirection.Dot(AxisWorld);

		if (InitialDirection.LengthSquared() <= 0.000001f) 
		{
			return false;
		}

		InitialDirection.Normalize();

		NewSession.InitialRotationDirection = InitialDirection;
	}
	//Translate와 Scale은 기존 축 드래그 평면을 사용한다.
	else {
		const FVector3 CameraPosition = LastCamera.View.Invert().Translation();
		FVector3 ViewDirection = InteractionPivotWorld - CameraPosition;

		if (ViewDirection.LengthSquared() <= std::numeric_limits<float>::epsilon()) 
		{
			return false;
		}

		ViewDirection.Normalize();

		// 선택 축을 포함하면서 카메라를 향하는 드래그 평면의 법선을 계산한다.
		FVector3 PlaneNormal = ViewDirection - AxisWorld * ViewDirection.Dot(AxisWorld);

		// 카메라 방향과 축이 거의 일치해서 평면 법선을 만들 수 없을 때의 대체 방향이다.
		if (PlaneNormal.LengthSquared() <= 0.000001f) 
		{
			const FVector3 Fallback = std::abs(AxisWorld.Dot(FVector3::UnitY)) < 0.95f ? FVector3::UnitY : FVector3::UnitX;
			PlaneNormal = Fallback - AxisWorld * Fallback.Dot(AxisWorld);
		}

		PlaneNormal.Normalize();

		NewSession.DragPlaneNormal = PlaneNormal;

		//Translate/Scale은 축 위의 시작 위치를 저장한다.
		if (!GetAxisParameterOnDragPlane(WorldRay,NewSession,NewSession.InitialAxisParameter)) 
		{
			return false;
		}
	}

	// 모든 초기화가 성공한 뒤에만 실제 드래그 세션으로 확정한다.
	DragSession = NewSession;

	// World 쪽에 Transform 편집 시작을 알린다. World는 SessionId를 기억하고, 이후 같은 SessionId의 Update/Commit/Cancel만 받는다.
	SendTransformEdit(
		NewSession.SessionId,
		ETransformEditPhase::Begin,
		NewSession.TargetHandle,
		NewSession.InitialWorld,
		NewSession.InitialTransformRevision);

	return true;
}

void FTransformGizmo::UpdateDrag(const FRay& WorldRay) {

	if (!DragSession.has_value()) 
	{
			return;
	}
	
	auto& Session = *DragSession;

	FMatrix DesiredWorld = Session.InitialWorld;

	// Rotation은 방향 벡터 사이의 각도로 계산한다.
	if (Session.ModifyMode == EModifyMode::Rotate) {
		// BeginDrag에서 사용한 것과 동일한 회전 평면.
		// 평면 중심 = 기즈모 Pivot
		// 평면 법선 = 선택한 회전축
		const FPlane RotationPlane
		{
			Session.InteractionPivotWorld.ToSimpleMath(),
			Session.AxisWorld.ToSimpleMath()
		};

		float Distance = 0.0f;

		// 현재 마우스 레이와 회전 평면의 교차점을 구한다.
		if (!WorldRay.Intersects(RotationPlane,Distance)|| Distance < 0.0f) 
		{
			return;
		}

		const FVector3 HitPosition
		{
			WorldRay.position + WorldRay.direction * Distance
		};

		// Pivot에서 현재 마우스 위치로 향하는 방향.
		FVector3 CurrentDirection = HitPosition - Session.InteractionPivotWorld;

		// 부동소수점 오차로 남을 수 있는 회전축 방향 성분을 제거한다.
		CurrentDirection = CurrentDirection - Session.AxisWorld * CurrentDirection.Dot(Session.AxisWorld);

		// 마우스가 Pivot과 너무 가까우면 유효한 방향을 만들 수 없다.
		if (CurrentDirection.LengthSquared() <= 0.000001f) 
		{
			return;
		}

		CurrentDirection.Normalize();

		// 시작 방향에서 현재 방향까지의 signed angle 계산.
		// Cross → 회전 방향
		// Dot   → 회전 각도
		const float SinAngle = Session.AxisWorld.Dot(Session.InitialRotationDirection.Cross(CurrentDirection));
		const float CosAngle = std::clamp(Session.InitialRotationDirection.Dot(CurrentDirection),-1.0f,1.0f);
		const float AngleDelta = std::atan2(SinAngle,CosAngle);

		const FQuat Rotation = FQuat::CreateFromAxisAngle(FVector(Session.AxisWorld.ToSimpleMath().x, Session.AxisWorld.ToSimpleMath().y, Session.AxisWorld.ToSimpleMath().z),AngleDelta);
		const FMatrix RotationMatrix = FMatrix::CreateFromQuaternion(Rotation);
		const FVector3 Pivot = Session.InteractionPivotWorld;

		// 오브젝트를 Pivot 원점으로 옮김
		//→ 회전
		// → 원래 Pivot 위치로 되돌림
		DesiredWorld = Session.InitialWorld * FMatrix::CreateTranslation(-Pivot) * RotationMatrix* FMatrix::CreateTranslation(Pivot);
	}
	// Translate와 Scale은 축 위의 이동량으로 계산한다.
	else {
		float CurrentAxisParameter = 0.0f;

		if (!GetAxisParameterOnDragPlane(WorldRay,Session,CurrentAxisParameter))
		{
			return;
		}

		const float Delta = CurrentAxisParameter- Session.InitialAxisParameter;

		if (Session.ModifyMode == EModifyMode::Translate) 
		{
			const FVector3 NewPosition =Session.InitialWorld.Translation()+ Session.AxisWorld * Delta;
			DesiredWorld.Translation(NewPosition);
		}
		else if (Session.ModifyMode== EModifyMode::Scale)
		{
			const float ScaleSpeed = std::max(100.0f * Session.WorkUnitsPerPixel, 0.0001f);
			const float ScaleFactor = std::max(0.01f,1.0f + Delta / ScaleSpeed);

			FVector3 InitialScale{};
			FQuat InitialRotation{};
			FVector3 InitialTranslation{};

			if (!Session.InitialWorld.Decompose(InitialScale,InitialRotation,InitialTranslation))
			{
				return;
			}

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
		else {
			return;
		}
	}

	// 계산한 월드 행렬을 World로 전송한다.
	SendTransformEdit(
		Session.SessionId,
		ETransformEditPhase::Update,
		Session.TargetHandle,
		DesiredWorld,
		Session.InitialTransformRevision);
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

	const EModifyMode CurrentMode = GizmoMode.HasValue() ? static_cast<EModifyMode>(GizmoMode.Peek()) : EModifyMode::None;

	const auto Submit = [&](const FMatrix& LocalTransform,FAssetHandle MeshHandle,FAssetHandle MaterialHandle)
		{
			Probe.GizmoProbes.emplace_back(FActorProbe{
				.World = LocalTransform * GizmoWorldTransform,
				.MeshHandle = MeshHandle,
				.MaterialHandle = MaterialHandle,
				.PipelineHandle = GizmoPipeline
				});
		};

	switch (CurrentMode) {
	case EModifyMode::Translate:
		Submit(CylinderXAxisTransform, CylinderMesh, RedMaterial);
		// Source transforms are Y-up, while the editor world is Z-up.  Keep the
		// gizmo's colors aligned with the world-space axis each handle moves.
		Submit(CylinderYAxisTransform, CylinderMesh, BlueMaterial);
		Submit(CylinderZAxisTransform, CylinderMesh, GreenMaterial);

		Submit(ConeXAxisTransform, ConeMesh, RedMaterial); // 해당 위치에 Cone 메쉬 사용
		Submit(ConeYAxisTransform, ConeMesh, BlueMaterial);
		Submit(ConeZAxisTransform, ConeMesh, GreenMaterial);
		break;

	case EModifyMode::Scale:
		Submit(CylinderXAxisTransform, CylinderMesh, RedMaterial);
		Submit(CylinderYAxisTransform, CylinderMesh, BlueMaterial);
		Submit(CylinderZAxisTransform, CylinderMesh, GreenMaterial);

		Submit(CubeXAxisTransform, CubeMesh, RedMaterial); // 해당 위치에 Cube 메쉬 사용
		Submit(CubeYAxisTransform, CubeMesh, BlueMaterial);
		Submit(CubeZAxisTransform, CubeMesh, GreenMaterial);
		break;

	case EModifyMode::Rotate:
		Submit(TorusXAxisTransform, GizmoTorusMesh, RedMaterial);
		Submit(TorusYAxisTransform, GizmoTorusMesh, BlueMaterial);
		Submit(TorusZAxisTransform, GizmoTorusMesh, GreenMaterial);
		break;

	default:
		break;
	}
}
