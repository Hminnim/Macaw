#include "PCH.h"
#include "FPropertyEditorContext.h"

#include "ImGui/imgui.h"
#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UAsset.h"
#include "Core/Base/TypeInfo.h"

#include <array>
#include <cstring>

bool FPropertyEditorContext::BeginCategory(const char* Label, bool bDefaultOpen) const {
    return ImGui::CollapsingHeader(Label, bDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

void FPropertyEditorContext::DrawDisabledText(const char* Text) const {
    ImGui::TextDisabled("%s", Text);
}

void FPropertyEditorContext::DrawButton(const char* Label, const std::function<void()>& OnClicked) const {
    if (ImGui::Button(Label)) {
        OnClicked();
    }
}

void FPropertyEditorContext::DrawBool(const char* Label, bool Value, const std::function<void(bool)>& Setter) const {
    if (ImGui::Checkbox(Label, &Value)) {
        Setter(Value);
    }
}

void FPropertyEditorContext::DrawFloat(const char* Label, float Value, float Speed, float Min, float Max, const std::function<void(float)>& Setter) const {
    if (ImGui::DragFloat(Label, &Value, Speed, Min, Max)) {
        Setter(Value);
    }
}

void FPropertyEditorContext::DrawVector3(const char* Label, const FVector3& Value, float Speed, float Min, float Max, const std::function<void(const FVector3&)>& Setter) const {
    FVector3 EditedValue = Value;
    if (ImGui::DragFloat3(Label, &EditedValue.x, Speed, Min, Max)) {
        Setter(EditedValue);
    }
}

void FPropertyEditorContext::DrawColor(const char* Label, const FVector4& Value, const std::function<void(const FVector4&)>& Setter) const {
    FVector4 EditedValue = Value;
    if (ImGui::ColorEdit4(Label, &EditedValue.x)) {
        Setter(EditedValue);
    }
}

void FPropertyEditorContext::DrawText(const char* Label, const FString& Value, const std::function<void(const FString&)>& Setter) const {
    std::array<char, 2048> TextBuffer{};
    const size_t CopyLength = std::min(Value.size(), TextBuffer.size() - 1);
    std::memcpy(TextBuffer.data(), Value.data(), CopyLength);
    if (ImGui::InputTextMultiline(Label, TextBuffer.data(), TextBuffer.size(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5.0f))) {
        Setter(TextBuffer.data());
    }
}

void FPropertyEditorContext::DrawTransform(const char* Label, const FTransform& Value, const std::function<void(const FTransform&)>& Setter) {
    const ImGuiID TransformId = ImGui::GetID(Label);
    if (EditingTransformId != TransformId) {
        UpdateTransformFields(Value);
        EditingTransformId = TransformId;
    }

    const bool bChanged =
        ImGui::DragFloat3("Position", &EditPosition.x, 0.1f) |
        ImGui::DragFloat3("Rotation", &EditRotation.x, 0.5f) |
        ImGui::DragFloat3("Scale", &EditScale.x, 0.05f) |
        ImGui::Checkbox("Absolute Location", &bAbsoluteLocation) |
        ImGui::Checkbox("Absolute Rotation", &bAbsoluteRotation) |
        ImGui::Checkbox("Absolute Scale", &bAbsoluteScale);
    if (bChanged) {
        Setter(BuildDesiredTransform());
    }
}

void FPropertyEditorContext::DrawReferencePicker(const char* Label, const char* Preview, bool bNoneSelected, const std::function<void()>& ClearSelection, const std::vector<FPropertyReferenceOption>& Options) const {
    if (!ImGui::BeginCombo(Label, Preview)) {
        return;
    }
    if (ImGui::Selectable("None", bNoneSelected)) {
        ClearSelection();
    }
    for (const FPropertyReferenceOption& Option : Options) {
        ImGui::PushID(Option.Id);
        if (ImGui::Selectable(Option.Label.c_str(), Option.bSelected)) {
            Option.OnSelected();
        }
        ImGui::PopID();
    }
    ImGui::EndCombo();
}

void FPropertyEditorContext::DrawAssetPicker(const char* Label, FAssetRegistry& Registry, const FTypeInfo& AssetType, FAssetHandle CurrentHandle, const std::function<void(FAssetHandle)>& Setter) const {
    UAsset* Current = Registry.ResolveAsset<UAsset>(CurrentHandle);
    if (Current != nullptr && !Current->GetTypeInfo()->IsA(&AssetType)) {
        Current = nullptr;
    }
    const FString PreviewName = Current != nullptr ? Current->GetAssetName() : FString("None");
    if (!ImGui::BeginCombo(Label, PreviewName.c_str())) {
        return;
    }
    if (ImGui::Selectable("None", Current == nullptr)) {
        Setter({});
    }
    for (UObject* Object : Registry.GetAssetList()) {
        if (Object == nullptr || !Object->GetTypeInfo()->IsA(&AssetType)) {
            continue;
        }
        auto* Asset = static_cast<UAsset*>(Object);
        const FString Name = Asset->GetAssetName();
        ImGui::PushID(Asset);
        if (ImGui::Selectable(Name.c_str(), Asset == Current)) {
            Setter(Registry.GetAsset(Asset->GetGuid()));
        }
        ImGui::PopID();
    }
    ImGui::EndCombo();
}

void FPropertyEditorContext::UpdateTransformFields(const FTransform& Transform) {
    EditPosition = Transform.GetPosition();
    EditRotation = Transform.GetRotation();
    EditScale = Transform.GetScale();
    bAbsoluteLocation = Transform.IsAbsoluteLocation();
    bAbsoluteRotation = Transform.IsAbsoluteRotation();
    bAbsoluteScale = Transform.IsAbsoluteScale();
}

FTransform FPropertyEditorContext::BuildDesiredTransform() const {
    FTransform Transform{ EditPosition, EditRotation, EditScale };
    Transform.SetAbsoluteLocation(bAbsoluteLocation);
    Transform.SetAbsoluteRotation(bAbsoluteRotation);
    Transform.SetAbsoluteScale(bAbsoluteScale);
    return Transform;
}
