#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "IEditorPanel.h"
#include "FEditorInfo.h"
#include "FPropertyEditorContext.h"
#include "Core/Channel/FStateChannel.h"
#include "Scene/AActor.h"
#include "Scene/FWorldEditorContext.h"
#include "Scene/Component/UActorComponent.h"
#include "Scene/Component/UBoxColliderComponent.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/USceneComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/Component/UTextRenderComponent.h"
#include "Scene/Component/UKTextRenderComponent.h"

// 목록, 선택, 구조 변경만 담당합니다. 타입별 Details는 Component::DrawPanels()로 위임합니다.
class FPropertyPanel : public IEditorPanel {
public:
    FPropertyPanel(
        FWorldEditorContext& InEditorContext,
        FStateChannel<uint8>::FReadWriter InGizmoMode,
        FStateChannel<uint8>::FReadWriter InGizmoCoordinateSpace)
        : EditorContext(&InEditorContext)
        , PropertyEditor(InEditorContext)
        , GizmoMode(std::move(InGizmoMode))
        , GizmoCoordinateSpace(std::move(InGizmoCoordinateSpace)) {
    }

    void DrawPanel() override {
        if (EditorContext == nullptr) return;

        ImGui::Begin("Property Window");
        AActor* Actor = EditorContext->GetSelectedActor();
        if (Actor == nullptr) {
            ImGui::TextDisabled("Select an actor to inspect its components.");
            ImGui::End();
            return;
        }

        DrawGizmoControls();
        DrawComponentList(*Actor);
        ImGui::Separator();

        if (UActorComponent* Component = EditorContext->GetSelectedComponent(); Component != nullptr && Component->GetOwner() == Actor) {
            ImGui::Text("Details: %s", GetComponentTypeName(*Component));
            ImGui::PushID(Component);
            Component->DrawPanels(PropertyEditor);
            ImGui::Separator();
            DrawDeleteButton(*Actor, *Component);
            ImGui::PopID();
        }
        else {
            ImGui::TextDisabled("Select a component.");
        }
        ImGui::End();
    }

private:
    static const char* GetComponentTypeName(const UActorComponent& Component) {
        return Component.GetTypeInfo()->TypeName.data();
    }

    void DrawGizmoControls() {
        ImGui::TextUnformatted("Gizmo Mode");
        int ModeIndex = static_cast<int>(GizmoMode.Read());
        bool bModeChanged = false;
        bModeChanged |= ImGui::RadioButton("Translate", &ModeIndex, static_cast<int>(EGizmoMode::Translate));
        ImGui::SameLine();
        bModeChanged |= ImGui::RadioButton("Rotate", &ModeIndex, static_cast<int>(EGizmoMode::Rotate));
        ImGui::SameLine();
        bModeChanged |= ImGui::RadioButton("Scale", &ModeIndex, static_cast<int>(EGizmoMode::Scale));
        if (bModeChanged) GizmoMode.Emplace(static_cast<uint8>(ModeIndex));

        ImGui::TextUnformatted("Coordinate Mode");
        int CoordinateSpaceIndex = static_cast<int>(GizmoCoordinateSpace.Read());
        bool bCoordinateSpaceChanged = false;
        bCoordinateSpaceChanged |= ImGui::RadioButton("World", &CoordinateSpaceIndex, static_cast<int>(EGizmoCoordinateSpace::World));
        ImGui::SameLine();
        bCoordinateSpaceChanged |= ImGui::RadioButton("Local", &CoordinateSpaceIndex, static_cast<int>(EGizmoCoordinateSpace::Local));
        if (bCoordinateSpaceChanged) GizmoCoordinateSpace.Emplace(static_cast<uint8>(CoordinateSpaceIndex));
        ImGui::Separator();
    }

    void DrawComponentList(AActor& Actor) {
        ImGui::TextUnformatted("Components");
        ImGui::SameLine();
        if (ImGui::Button("+ Add Component")) ImGui::OpenPopup("AddComponentPopup");

        if (ImGui::BeginPopup("AddComponentPopup")) {
            ImGui::TextDisabled("Add to selected actor");
            ImGui::Separator();
            if (ImGui::MenuItem("Scene Component")) AddSceneComponent<USceneComponent>(Actor);
            if (ImGui::MenuItem("Static Mesh Component")) AddSceneComponent<UStaticMeshComponent>(Actor);
            if (ImGui::MenuItem("Camera Component")) AddSceneComponent<UCameraComponent>(Actor);
            if (ImGui::MenuItem("Box Collider Component")) AddSceneComponent<UBoxColliderComponent>(Actor);
            if (ImGui::MenuItem("Text Render Component")) AddSceneComponent<UTextRenderComponent>(Actor);
            if (ImGui::MenuItem("Korean Text Render Component")) AddSceneComponent<UKTextRenderComponent>(Actor);
            ImGui::EndPopup();
        }

        ImGui::BeginChild("ComponentList", ImVec2(0.0f, 180.0f), true);
        for (const std::unique_ptr<UActorComponent>& Component : Actor.GetComponents()) {
            auto* SceneComponent = static_cast<USceneComponent*>(Component.get());

            if (SceneComponent == nullptr) {
                DrawComponentNode(*Component, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            }
            else if (SceneComponent->GetParent() == nullptr || SceneComponent->GetParent()->GetOwner() != &Actor) {
                DrawSceneComponentTree(Actor, *SceneComponent);
            }
        }
        ImGui::EndChild();
    }

    void DrawSceneComponentTree(AActor& Actor, USceneComponent& Component) {
        const bool bHasOwnedChildren = std::ranges::any_of(Component.GetChildren(), [&Actor](const TObjectRef<USceneComponent>& Child) {
            return Child.Get() != nullptr && Child.Get()->GetOwner() == &Actor;
        });
        ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!bHasOwnedChildren) Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (EditorContext->GetSelectedComponent() == &Component) Flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::PushID(&Component);
        const bool bOpen = ImGui::TreeNodeEx("Component", Flags, "%s", GetComponentTypeName(Component));
        if (ImGui::IsItemClicked()) EditorContext->SetSelectedComponent(&Component);
        if (bHasOwnedChildren && bOpen) {
            for (const TObjectRef<USceneComponent>& Child : Component.GetChildren()) {
                if (USceneComponent* ChildComponent = Child.Get(); ChildComponent != nullptr && ChildComponent->GetOwner() == &Actor) {
                    DrawSceneComponentTree(Actor, *ChildComponent);
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void DrawComponentNode(UActorComponent& Component, ImGuiTreeNodeFlags Flags) {
        if (EditorContext->GetSelectedComponent() == &Component) Flags |= ImGuiTreeNodeFlags_Selected;
        ImGui::PushID(&Component);
        ImGui::TreeNodeEx("Component", Flags, "%s", GetComponentTypeName(Component));
        if (ImGui::IsItemClicked()) EditorContext->SetSelectedComponent(&Component);
        ImGui::PopID();
    }

    template<typename T>
    void AddSceneComponent(AActor& Actor) {
        static_assert(std::is_base_of_v<USceneComponent, T>);
        T* NewComponent = Actor.AddComponent<T>();
        if (USceneComponent* Root = Actor.GetRootComponent()) NewComponent->AttachToComponent(Root);
        else Actor.SetRootComponent(NewComponent);
        EditorContext->SetSelectedComponent(NewComponent);
    }

    void DrawDeleteButton(AActor& Actor, UActorComponent& Component) {
        auto* SceneComponent = dynamic_cast<USceneComponent*>(&Component);
        const bool bDeletingRootWithoutReplacement = SceneComponent == Actor.GetRootComponent() && FindReplacementRoot(Actor, SceneComponent) == nullptr;
        if (bDeletingRootWithoutReplacement) ImGui::BeginDisabled();
        const bool bDeleteClicked = ImGui::Button("Delete Component");
        if (bDeletingRootWithoutReplacement) {
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("Add another Scene Component before deleting the root.");
        }
        if (!bDeleteClicked || bDeletingRootWithoutReplacement) return;
        if (SceneComponent == Actor.GetRootComponent()) {
            USceneComponent* NewRoot = FindReplacementRoot(Actor, SceneComponent);
            NewRoot->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
            Actor.SetRootComponent(NewRoot);
        }
        Actor.DestroyComponent(&Component);
        EditorContext->SetSelectedActor(&Actor);
    }

    static USceneComponent* FindReplacementRoot(AActor& Actor, USceneComponent* Excluded) {
        for (const std::unique_ptr<UActorComponent>& Candidate : Actor.GetComponents()) {
            auto* SceneComponent = dynamic_cast<USceneComponent*>(Candidate.get());
            if (SceneComponent != nullptr && SceneComponent != Excluded) return SceneComponent;
        }
        return nullptr;
    }

private:
    FWorldEditorContext* EditorContext = nullptr;
    FPropertyEditorContext PropertyEditor;
    FStateChannel<uint8>::FReadWriter GizmoMode;
    FStateChannel<uint8>::FReadWriter GizmoCoordinateSpace;
};
