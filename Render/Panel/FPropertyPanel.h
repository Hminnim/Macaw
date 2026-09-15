#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "IEditorPanel.h"
#include "FEditorInfo.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "../../Scene/FWorldEditorContext.h"
#include "../../Scene/Component/USceneComponent.h"

class FPropertyPanel : public IEditorPanel {
public:
	FPropertyPanel(
		FWorldEditorContext& InEditorContext,
		FStateChannel<uint8>::FReadWriter InGizmoMode,
		FStateChannel<uint8>::FReadWriter InGizmoCoordinateSpace)
		: EditorContext(&InEditorContext)
		, GizmoMode(std::move(InGizmoMode))
		, GizmoCoordinateSpace(std::move(InGizmoCoordinateSpace)) {
	}

	void DrawPanel() override {
		if (EditorContext == nullptr) {
			return;
		}

		USceneComponent* Target = EditorContext->GetSelectedTransformTarget();
		if (Target == nullptr) {
			bEditingTransform = false;
			EditingTransformTarget = nullptr;
			return;
		}

		if (bEditingTransform && EditingTransformTarget != Target) {
			bEditingTransform = false;
			EditingTransformTarget = nullptr;
		}

		if (!bEditingTransform) {
			UpdateTransformFields(Target->GetRelativeTransform());
		}

		ImGui::Begin("Property Window");

		ImGui::Text("Gizmo Mode");
		CurrentGizmoMode = static_cast<EGizmoMode>(GizmoMode.Read());
		int ModeIndex = static_cast<int>(CurrentGizmoMode);
		bool bGizmoChanged = false;

		bGizmoChanged |= ImGui::RadioButton("Translate", &ModeIndex, 0);
		ImGui::SameLine();
		bGizmoChanged |= ImGui::RadioButton("Rotate", &ModeIndex, 1);
		ImGui::SameLine();
		bGizmoChanged |= ImGui::RadioButton("Scale", &ModeIndex, 2);

		if (bGizmoChanged) {
			CurrentGizmoMode = static_cast<EGizmoMode>(ModeIndex);
			GizmoMode.Emplace(static_cast<uint8>(CurrentGizmoMode));
		}

		ImGui::SameLine();
		ImGui::Text("Coordinate");
		int CoordinateSpaceIndex = static_cast<int>(GizmoCoordinateSpace.Read());
		bool bCoordinateSpaceChanged = false;
		
		bCoordinateSpaceChanged |= ImGui::RadioButton("World", &CoordinateSpaceIndex, static_cast<int>(EGizmoCoordinateSpace::World));
		ImGui::SameLine();
		bCoordinateSpaceChanged |= ImGui::RadioButton("Local", &CoordinateSpaceIndex, static_cast<int>(EGizmoCoordinateSpace::Local));
		if (bCoordinateSpaceChanged) {
			GizmoCoordinateSpace.Emplace(static_cast<uint8>(CoordinateSpaceIndex));
		}
		ImGui::Separator();

		ImGui::Text("Relative Transform");

		const bool bPositionModified = ImGui::DragFloat3("Position", &EditPosition.x, 0.1f);
		const bool bPositionActivated = ImGui::IsItemActivated();
		const bool bPositionDeactivated = ImGui::IsItemDeactivatedAfterEdit();

		const bool bRotationModified = ImGui::DragFloat3("Rotation", &EditRotation.x, 0.5f);
		const bool bRotationActivated = ImGui::IsItemActivated();
		const bool bRotationDeactivated = ImGui::IsItemDeactivatedAfterEdit();

		const bool bScaleModified = ImGui::DragFloat3("Scale", &EditScale.x, 0.05f);
		const bool bScaleActivated = ImGui::IsItemActivated();
		const bool bScaleDeactivated = ImGui::IsItemDeactivatedAfterEdit();

		bool bAbsoluteTransformChanged = false;
		bAbsoluteTransformChanged |= ImGui::Checkbox("Absolute Location", &bAbsoluteLocation);
		bAbsoluteTransformChanged |= ImGui::Checkbox("Absolute Rotation", &bAbsoluteRotation);
		bAbsoluteTransformChanged |= ImGui::Checkbox("Absolute Scale", &bAbsoluteScale);

		const bool bTransformActivated = bPositionActivated || bRotationActivated || bScaleActivated;
		const bool bTransformModified = bPositionModified || bRotationModified || bScaleModified;
		const bool bTransformDeactivated = bPositionDeactivated || bRotationDeactivated || bScaleDeactivated;

		if (bTransformActivated) {
			bEditingTransform = true;
			EditingTransformTarget = Target;
		}

		if (bTransformModified && bEditingTransform && EditingTransformTarget == Target) {
			Target->SetRelativeTransform(BuildDesiredTransform());
		}

		if (bAbsoluteTransformChanged && !bEditingTransform) {
			Target->SetRelativeTransform(BuildDesiredTransform());
		}

		if (bTransformDeactivated) {
			bEditingTransform = false;
			EditingTransformTarget = nullptr;
		}

		ImGui::End();
	}

private:
	void UpdateTransformFields(const FTransform& Transform) {
		EditPosition = Transform.GetPosition();
		EditRotation = Transform.GetRotation();
		EditScale = Transform.GetScale();
		bAbsoluteLocation = Transform.IsAbsoluteLocation();
		bAbsoluteRotation = Transform.IsAbsoluteRotation();
		bAbsoluteScale = Transform.IsAbsoluteScale();
	}

	FTransform BuildDesiredTransform() const {
		FTransform Transform{ EditPosition, EditRotation, EditScale };
		Transform.SetAbsoluteLocation(bAbsoluteLocation);
		Transform.SetAbsoluteRotation(bAbsoluteRotation);
		Transform.SetAbsoluteScale(bAbsoluteScale);
		return Transform;
	}

private:
	FWorldEditorContext* EditorContext = nullptr;
	FStateChannel<uint8>::FReadWriter GizmoMode;
	FStateChannel<uint8>::FReadWriter GizmoCoordinateSpace;

	USceneComponent* EditingTransformTarget = nullptr;
	bool bEditingTransform = false;
	FVector3 EditPosition{};
	FRotator EditRotation{};
	FVector3 EditScale{ 1.0f, 1.0f, 1.0f };
	bool bAbsoluteLocation = false;
	bool bAbsoluteRotation = false;
	bool bAbsoluteScale = false;
	EGizmoMode CurrentGizmoMode = EGizmoMode::Translate;
};
