#pragma once

#include <array>
#include <cstdint>
#include <d3d11.h>
#include <optional>

#include "../../Core/Asset/FAssetRegistry.h"
#include "../../Core/Asset/UMaterial.h"
#include "../../Core/Asset/UMesh.h"
#include "../../Core/Base/FRenderProbe.h"
#include "../../Core/Channel/FStateChannel.h"
#include "../../FEditorSelectionState.h"
#include "../../Scene/FWorldEditorContext.h"
#include "../../FTransformEditRequestMessage.h"
#include "../Pipeline/UPipeline.h"
#include "../RenderWindowInfo.h"

#include "../../FMouseInput.h"
#include "../../FKeyboardInput.h"

class FTransformGizmo {
	enum class EAxis : std::uint8_t {
		None,
		X,
		Y,
		Z
	};

	enum class EModifyMode : uint8 {
		Translate, Rotate, Scale, None
	};

	struct FAxisHitProxy {
		EAxis Axis = EAxis::None;
		FVector3 Center{};
		FVector3 Extent{};
	};

	struct FAxisHit {
		EAxis Axis = EAxis::None;
		float Distance = 0.0f;
	};

	struct FDragSession {
		std::uint64_t SessionId = 0;
		FObjectHandle TargetHandle{};
		FMatrix InitialWorld{ FMatrix::Identity };
		FVector3 AxisWorld{};
		FVector3 InteractionPivotWorld{};
		FVector3 DragPlaneNormal{};
		float InitialAxisParameter = 0.0f;
		std::uint64_t InitialTransformRevision = 0;
		EAxis DragAxis = EAxis::None;
		EModifyMode ModifyMode = EModifyMode::None;
		FVector3 InitialRotationDirection{};
		float WorkUnitsPerPixel = 1.0f;
	};

public:
	FTransformGizmo() = default;
	~FTransformGizmo() = default;

	FTransformGizmo(const FTransformGizmo&) = delete;
	FTransformGizmo& operator=(const FTransformGizmo&) = delete;

	FTransformGizmo(FTransformGizmo&&) = default;
	FTransformGizmo& operator=(FTransformGizmo&&) = default;

public:
	void Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, FStateChannel<RenderWindowInfo>::FReader InWindowInfoReader, FWorldEditorContext& InEditorContext);

	void ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool bMouseCapturedByUI);
	void Update(const CameraProbe& Camera);
	void Render(FRenderProbe& Probe);

	FStateChannel<uint8>::FReadWriter GetGizmoMode() { return GizmoModeChannel.GetReadWriter(); }
private:

	void SetTranslate(const FVector3& Pivot, float WorldUnitsPerPixel);
	void SetScale(const FVector3& Pivot, float WorldUnitsPerPixel);
	void SetRotate(const FVector3& Pivot, float WorldUnitsPerPixel);

	FAssetHandle GetAxisMaterial(EAxis Axis) const;
	void AddRenderPart(EAxis Axis,const FMatrix& LocalTransform,FAssetHandle MeshHandle);

	void UpdateBoundsInGizmoSpace(const FEditorSelectionState& Selection, FVector3& OutCenter, FVector3& OutExtent) const;

	std::optional<FRay> MakeWorldRay(const POINT& ScreenPosition) const;
	std::optional<FAxisHit> HitTest(const FRay& WorldRay) const;

	bool BeginDrag(EAxis Axis, const FRay& WorldRay);
	void UpdateDrag(const FRay& WorldRay);
	void EndDrag(bool bCancel);
	bool GetAxisParameterOnDragPlane(const FRay& WorldRay, const FDragSession& Session, float& OutParameter) const;
	FVector3 GetWorldAxis(EAxis Axis) const;

	void SendTransformEdit(std::uint64_t SessionId, ETransformEditPhase Phase, FObjectHandle TargetHandle, const FMatrix& DesiredWorld, std::uint64_t ExpectedTransformRevision);

private:
	static constexpr float ShaftLengthPixels = 66.0f;
	static constexpr float ConeLengthPixels = 22.0f;
	static constexpr float ShaftRadiusPixels = 6.0f;
	static constexpr float ConeRadiusPixels = 13.0f;
	static constexpr float PickRadiusPixels = 10.0f;
	static constexpr float BoundsGapPixels = 2.0f;

	float CurrentRingRadius = 0.0f;
	float CurrentRingPickHalfWidth = 0.0f;

	FAssetHandle CylinderMesh{};
	FAssetHandle ConeMesh{};
	FAssetHandle CubeMesh{};
	FAssetHandle GizmoTorusMesh{};

	FAssetHandle RedMaterial{};
	FAssetHandle GreenMaterial{};
	FAssetHandle BlueMaterial{};

	FAssetHandle GizmoPipeline{};

	FMatrix CylinderXAxisTransform{ FMatrix::Identity };
	FMatrix CylinderYAxisTransform{ FMatrix::Identity };
	FMatrix CylinderZAxisTransform{ FMatrix::Identity };

	FMatrix ConeXAxisTransform{ FMatrix::Identity };
	FMatrix ConeYAxisTransform{ FMatrix::Identity };
	FMatrix ConeZAxisTransform{ FMatrix::Identity };

	FMatrix CubeXAxisTransform{ FMatrix::Identity };
	FMatrix CubeYAxisTransform{ FMatrix::Identity };
	FMatrix CubeZAxisTransform{ FMatrix::Identity };

	FMatrix TorusXAxisTransform{ FMatrix::Identity };
	FMatrix TorusYAxisTransform{ FMatrix::Identity };
	FMatrix TorusZAxisTransform{ FMatrix::Identity };

	FMatrix GizmoWorldTransform{ FMatrix::Identity };
	FVector3 BoundsCenterInGizmoSpace{};
	std::array<FAxisHitProxy, 3> AxisHitProxies{};

	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader{};
	FWorldEditorContext* EditorContext = nullptr;
	FStateChannel<uint8> GizmoModeChannel{};
	FStateChannel<uint8>::FReadWriter GizmoMode{};

	FEditorSelectionState CurrentSelection{};
	CameraProbe LastCamera{};

	std::optional<FDragSession> DragSession;

	bool bVisible = false;
	bool bHasCamera = false;

	float CurrentWorkUnitsPerPixel{ 1.0f };
};
