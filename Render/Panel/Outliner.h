#pragma once

#include "ImGui/imgui.h"
#include "Render/Panel/IEditorPanel.h"
#include "Scene/UWorld.h"

class FWorldEditorContext;

class FOutlinerPanel : public IEditorPanel {
public:
    FOutlinerPanel(UWorld& InWorld, FWorldEditorContext& InEditorContext);

    void DrawPanel() override;

private:
    bool MatchesActor(const AActor& Actor) const;
    bool IsActorAttachedTo(const AActor& Actor, const AActor& ParentActor) const;
    bool IsRootActor(const AActor& Actor) const;
    bool HasActorChildren(const AActor& Actor) const;
    void DrawActorDragSource(AActor& Actor);
    void AcceptActorChildDrop(AActor& ParentActor);
    void DrawRootActorDropTarget();
    void HandleDeleteShortcut();
    void DrawActor(AActor& Actor);
    void DrawRootActors();

    UWorld* World;
    FWorldEditorContext* EditorContext;
    ImGuiTextFilter ActorFilter;
};
