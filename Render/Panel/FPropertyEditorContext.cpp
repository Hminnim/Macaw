#include "PCH.h"
#include "FPropertyEditorContext.h"

#include "ImGui/imgui.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Component/UActorComponent.h"
#include "Scene/Component/UBoxColliderComponent.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/UMeshComponent.h"
#include "Scene/Component/UPrimitiveComponent.h"
#include "Scene/Component/USceneComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/Component/UTextRenderComponent.h"
#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UMaterial.h"
#include "Core/Asset/UMesh.h"
#include "Core/Asset/UFont.h"
#include "Render/Pipeline/UPipeline.h"

#include <array>
#include <cstring>

namespace {
    const char* GetComponentTypeName(const UActorComponent& Component) {
        return Component.GetTypeInfo()->TypeName.data();
    }
}

FPropertyEditorContext::FPropertyEditorContext(FWorldEditorContext& InEditorContext)
    : EditorContext(&InEditorContext) {
}

void FPropertyEditorContext::DrawActorComponentProperties(UActorComponent& Component) {
    bool bActive = Component.IsActive();
    if (ImGui::Checkbox("Active", &bActive)) {
        Component.SetActive(bActive);
    }
}

void FPropertyEditorContext::DrawSceneComponentProperties(USceneComponent& Component) {
    AActor* Actor = Component.GetOwner();
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawTransform(Component);
    }
    if (Actor != nullptr && ImGui::CollapsingHeader("Attachment", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawAttachment(*Actor, Component);
    }
}

void FPropertyEditorContext::DrawPrimitiveComponentProperties(UPrimitiveComponent& Component) {
    bool bVisible = Component.IsVisible();
    if (ImGui::Checkbox("Visible", &bVisible)) {
        Component.SetVisible(bVisible);
    }
}

void FPropertyEditorContext::DrawMeshComponentProperties(UMeshComponent& Component) {
    DrawAssetPicker<UMesh>("Mesh", Component, Component.GetMeshHandle(), [&Component](FAssetHandle Handle) {
        Component.SetMeshHandle(Handle);
    });
}

void FPropertyEditorContext::DrawStaticMeshComponentProperties(UStaticMeshComponent& Component) {
    DrawAssetPicker<UMaterial>("Material", Component, Component.GetMaterialHandle(), [&Component](FAssetHandle Handle) {
        Component.SetMaterialHandle(Handle);
    });
    DrawAssetPicker<UPipeline>("Pipeline", Component, Component.GetPipelineHandle(), [&Component](FAssetHandle Handle) {
        Component.SetPipelineHandle(Handle);
    });
}

void FPropertyEditorContext::DrawCameraComponentProperties(UCameraComponent& Component) {
    float FOVDegrees = DirectX::XMConvertToDegrees(Component.GetFOV());
    if (ImGui::DragFloat("FOV (Degrees)", &FOVDegrees, 0.1f, 1.0f, 179.0f)) {
        Component.SetFOV(DirectX::XMConvertToRadians(FOVDegrees));
    }
    float AspectRatio = Component.GetAspectRatio();
    if (ImGui::DragFloat("Aspect Ratio", &AspectRatio, 0.01f, 0.01f, 100.0f)) {
        Component.SetAspectRatio(AspectRatio);
    }
    float NearPlane = Component.GetNearPlane();
    if (ImGui::DragFloat("Near Plane", &NearPlane, 0.01f, 0.001f, Component.GetFarPlane() - 0.001f)) {
        Component.SetNearPlane(NearPlane);
    }
    float FarPlane = Component.GetFarPlane();
    if (ImGui::DragFloat("Far Plane", &FarPlane, 1.0f, Component.GetNearPlane() + 0.001f, 1000000.0f)) {
        Component.SetFarPlane(FarPlane);
    }
}

void FPropertyEditorContext::DrawCollisionComponentProperties(UCollisionComponent& Component) {
    bool bCollisionEnabled = Component.IsCollisionEnabled();
    if (ImGui::Checkbox("Collision Enabled", &bCollisionEnabled)) {
        Component.SetCollisionEnabled(bCollisionEnabled);
    }
}

void FPropertyEditorContext::DrawBoxColliderComponentProperties(UBoxColliderComponent& Component) {
    FVector3 Extent = Component.GetExtent();
    if (ImGui::DragFloat3("Extent", &Extent.x, 0.05f, 0.001f, FLT_MAX)) {
        Component.SetExtent(Extent);
    }

    AActor* Actor = Component.GetOwner();
    if (Actor == nullptr) return;
    UMeshComponent* CurrentMesh = Component.GetMeshComponent();
    const char* Preview = CurrentMesh != nullptr ? GetComponentTypeName(*CurrentMesh) : "None";
    if (ImGui::BeginCombo("Source Mesh Component", Preview)) {
        if (ImGui::Selectable("None", CurrentMesh == nullptr)) {
            Component.SetMeshComponent(nullptr);
        }
        for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
            auto* Mesh = dynamic_cast<UMeshComponent*>(Candidate.get());
            if (Mesh == nullptr) continue;
            ImGui::PushID(Mesh);
            if (ImGui::Selectable(GetComponentTypeName(*Mesh), CurrentMesh == Mesh)) {
                Component.SetMeshComponent(Mesh);
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Build Bounds From Mesh")) {
        Component.BuildBoundsFromMesh();
    }
}

void FPropertyEditorContext::DrawTextRenderComponentProperties(UTextRenderComponent& Component) {
    std::array<char, 2048> TextBuffer{};
    const FString& Text = Component.GetText();
    const size_t CopyLength = std::min(Text.size(), TextBuffer.size() - 1);
    std::memcpy(TextBuffer.data(), Text.data(), CopyLength);
    if (ImGui::InputTextMultiline("Text", TextBuffer.data(), TextBuffer.size(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5.0f))) {
        Component.SetText(TextBuffer.data());
    }
    FVector4 Color = Component.GetColor();
    if (ImGui::ColorEdit4("Color", &Color.x)) {
        Component.SetColor(Color);
    }
    float CharacterHeight = Component.GetCharacterHeight();
    if (ImGui::DragFloat("Character Height", &CharacterHeight, 0.01f, 0.001f, FLT_MAX)) {
        Component.SetCharacterHeight(CharacterHeight);
    }
    float LetterSpacing = Component.GetLetterSpacing();
    if (ImGui::DragFloat("Letter Spacing", &LetterSpacing, 0.01f)) {
        Component.SetLetterSpacing(LetterSpacing);
    }
    float LineSpacing = Component.GetLineSpacing();
    if (ImGui::DragFloat("Line Spacing", &LineSpacing, 0.01f)) {
        Component.SetLineSpacing(LineSpacing);
    }
    DrawAssetPicker<UFont>("Font", Component, Component.GetFontHandle(), [&Component](FAssetHandle Handle) {
        Component.SetFontHandle(Handle);
    });
    DrawAssetPicker<UPipeline>("Text Pipeline", Component, Component.GetPipelineHandle(), [&Component](FAssetHandle Handle) {
        Component.SetPipelineHandle(Handle);
    });
}

void FPropertyEditorContext::DrawTransform(USceneComponent& Component) {
    if (EditingTransformTarget != &Component) {
        UpdateTransformFields(Component.GetRelativeTransform());
        EditingTransformTarget = &Component;
    }

    const bool bChanged =
        ImGui::DragFloat3("Position", &EditPosition.x, 0.1f) or
        ImGui::DragFloat3("Rotation", &EditRotation.x, 0.5f) or
        ImGui::DragFloat3("Scale", &EditScale.x, 0.05f) or
        ImGui::Checkbox("Absolute Location", &bAbsoluteLocation) or
        ImGui::Checkbox("Absolute Rotation", &bAbsoluteRotation) or
        ImGui::Checkbox("Absolute Scale", &bAbsoluteScale);

    if (bChanged) {
        Component.SetRelativeTransform(BuildDesiredTransform());
    }
}

void FPropertyEditorContext::DrawAttachment(AActor& Actor, USceneComponent& Component) {
    if (Actor.GetRootComponent() == &Component) {
        ImGui::TextDisabled("Root Component");
        return;
    }

    DrawParentPicker(Actor, Component);
    if (ImGui::Button("Make Root Component")) {
        Component.DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
        Actor.SetRootComponent(&Component);
    }
}

void FPropertyEditorContext::DrawParentPicker(AActor& Actor, USceneComponent& Component) {
    const char* Preview = Component.GetParent() != nullptr ? GetComponentTypeName(*Component.GetParent()) : "None";
    if (!ImGui::BeginCombo("Parent", Preview)) return;
    if (ImGui::Selectable("None", Component.GetParent() == nullptr)) {
        Component.DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
    }
    for (const std::unique_ptr<UActorComponent>& Candidate : Actor.GetComponents()) {
        auto* Parent = dynamic_cast<USceneComponent*>(Candidate.get());
        if (Parent == nullptr || Parent == &Component) continue;
        ImGui::PushID(Parent);
        if (ImGui::Selectable(GetComponentTypeName(*Parent), Component.GetParent() == Parent)) {
            Component.AttachToComponent(Parent, EAttachmentTransformRule::KeepWorldTransform);
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

template<typename TAsset, typename TSetter>
void FPropertyEditorContext::DrawAssetPicker(const char* Label, UActorComponent& Component, FAssetHandle CurrentHandle, TSetter&& SetHandle) {
    AActor* Actor = Component.GetOwner();
    UWorld* World = Actor != nullptr ? Actor->GetWorld() : nullptr;
    FAssetRegistry* Registry = World != nullptr ? World->GetAssetRegistry() : nullptr;
    if (Registry == nullptr) {
        ImGui::TextDisabled("%s: Asset registry unavailable", Label);
        return;
    }
    const TAsset* Current = Registry->ResolveAsset<TAsset>(CurrentHandle);
    const FString PreviewName = Current != nullptr ? Current->GetAssetName() : FString("None");
    
    if (!ImGui::BeginCombo(Label, PreviewName.c_str())) return;
    if (ImGui::Selectable("None", Current == nullptr)) SetHandle({});

    for (UObject* Object : Registry->GetAssetList()) {
        if (Object == nullptr || !Object->GetTypeInfo()->IsA(TAsset::StaticTypeInfo())) continue;
        auto* Asset = static_cast<TAsset*>(Object);
        const FString& Name = Asset->GetAssetName();
        ImGui::PushID(Asset);
        if (ImGui::Selectable(Name.c_str(), Asset == Current)) {
            SetHandle(Registry->GetAsset(Name)); // 에셋이 중복 생성 될 때 GUID 가 서로 맞지 않는다 
        }
        ImGui::PopID();
    }
    ImGui::EndCombo();
}
