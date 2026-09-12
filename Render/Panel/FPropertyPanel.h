#pragma once

#include <optional>

#include "PCH.h"
#include "ImGui/imgui.h"
#include "IEditorPanel.h"
#include "FEditorInfo.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "../../FEditorSelectionState.h"
#include "../../FTransformEditRequestMessage.h"
#include "../../Scene/FWorldEditorContext.h"

class FPropertyPanel : public IEditorPanel {
	struct FTransformEditSession {
		std::uint64_t SessionId = 0;
		FObjectHandle TargetHandle{};
		FMatrix InitialWorld{ FMatrix::Identity };
		std::uint64_t InitialTransformRevision = 0;
	};

public:
	FPropertyPanel(
		FWorldEditorContext& InEditorContext,
		FStateChannel<uint8>::FReadWriter InGizmoMode)
		: EditorContext(&InEditorContext)
		, GizmoMode(std::move(InGizmoMode)) {
	}

	void DrawPanel() override {
		if (EditorContext == nullptr || EditorContext->GetSelectionState() == nullptr) {
			CancelTransformEdit();
			return;
		}

		const FEditorSelectionState& Selection = *EditorContext->GetSelectionState();
		if (!Selection.TransformTargetHandle.IsValid()) {
			CancelTransformEdit();
			return;
		}

		if (ActiveTransformEdit.has_value() && ActiveTransformEdit->TargetHandle != Selection.TransformTargetHandle) {
			CancelTransformEdit();
		}

		if (!ActiveTransformEdit.has_value() && !UpdateTransformFields(Selection.TargetWorld)) {
			return;
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

		ImGui::Text("Transform");

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

		if (bTransformActivated && !ActiveTransformEdit.has_value()) {
			BeginTransformEdit(Selection);
		}

		if (bTransformModified && ActiveTransformEdit.has_value()) {
			UpdateTransformEdit();
		}

		if (bTransformDeactivated && ActiveTransformEdit.has_value()) {
			CommitTransformEdit();
		}

		ImGui::End();
	}

private:
	bool UpdateTransformFields(FMatrix WorldTransform) {
		FQuat Rotation{};
		if (!WorldTransform.Decompose(EditScale, Rotation, EditPosition)) {
			return false;
		}

		auto euler = Rotation.ToEuler();
		EditRotation = FVector3(euler.x, euler.y, euler.z);
		return true;
	}

	FMatrix BuildDesiredWorld() const {
		return FMatrix::CreateScale(EditScale)
			* FMatrix::CreateFromYawPitchRoll(EditRotation.y, EditRotation.x, EditRotation.z)
			* FMatrix::CreateTranslation(EditPosition);
	}

	bool SendTransformEdit(
		std::uint64_t SessionId,
		ETransformEditPhase Phase,
		FObjectHandle TargetHandle,
		const FMatrix& DesiredWorld,
		std::uint64_t ExpectedTransformRevision) {
		return EditorContext->GetEditorToWorldSender().TryEmplace<FTransformEditRequestMessage>(
			SessionId,
			Phase,
			TargetHandle,
			DesiredWorld,
			ExpectedTransformRevision);
	}

	void BeginTransformEdit(const FEditorSelectionState& Selection) {
		const std::uint64_t SessionId = AcquireTransformEditSessionId();
		if (!SendTransformEdit(
			SessionId,
			ETransformEditPhase::Begin,
			Selection.TransformTargetHandle,
			Selection.TargetWorld,
			Selection.TransformRevision)) {
			return;
		}

		ActiveTransformEdit = FTransformEditSession{
			.SessionId = SessionId,
			.TargetHandle = Selection.TransformTargetHandle,
			.InitialWorld = Selection.TargetWorld,
			.InitialTransformRevision = Selection.TransformRevision
		};
	}

	void UpdateTransformEdit() {
		const FTransformEditSession& Session = *ActiveTransformEdit;
		SendTransformEdit(
			Session.SessionId,
			ETransformEditPhase::Update,
			Session.TargetHandle,
			BuildDesiredWorld(),
			Session.InitialTransformRevision);
	}

	void CommitTransformEdit() {
		const FTransformEditSession& Session = *ActiveTransformEdit;
		SendTransformEdit(
			Session.SessionId,
			ETransformEditPhase::Commit,
			Session.TargetHandle,
			BuildDesiredWorld(),
			Session.InitialTransformRevision);
		ActiveTransformEdit.reset();
	}

	void CancelTransformEdit() {
		if (!ActiveTransformEdit.has_value()) {
			return;
		}

		const FTransformEditSession& Session = *ActiveTransformEdit;
		SendTransformEdit(
			Session.SessionId,
			ETransformEditPhase::Cancel,
			Session.TargetHandle,
			Session.InitialWorld,
			Session.InitialTransformRevision);
		ActiveTransformEdit.reset();
	}

private:
	FWorldEditorContext* EditorContext = nullptr;
	FStateChannel<uint8>::FReadWriter GizmoMode;

	std::optional<FTransformEditSession> ActiveTransformEdit;
	FVector3 EditPosition{};
	FVector3 EditRotation{};
	FVector3 EditScale{ 1.0f, 1.0f, 1.0f };
	EGizmoMode CurrentGizmoMode = EGizmoMode::Translate;
};
