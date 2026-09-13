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
		FStateChannel<uint8>::FReadWriter InGizmoMode)
		: EditorContext(&InEditorContext)
		, GizmoMode(std::move(InGizmoMode)) {
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
	}

	FTransform BuildDesiredTransform() const {
		return FTransform{ EditPosition, EditRotation, EditScale };
	}

private:
	FWorldEditorContext* EditorContext = nullptr;
	FStateChannel<uint8>::FReadWriter GizmoMode;

	USceneComponent* EditingTransformTarget = nullptr;
	bool bEditingTransform = false;
	FVector3 EditPosition{};
	FRotator EditRotation{};
	FVector3 EditScale{ 1.0f, 1.0f, 1.0f };
	EGizmoMode CurrentGizmoMode = EGizmoMode::Translate;
};
