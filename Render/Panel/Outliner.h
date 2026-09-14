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
    bool MatchesFolder(const Folder& FolderRecord) const;
    bool MatchesActor(const AActor& Actor) const;
    bool IsActorAttachedTo(const AActor& Actor, const AActor& ParentActor) const;
    bool IsActorRootInFolder(const AActor& Actor) const;
    bool HasChildFolders(const Folder& FolderRecord) const;
    bool HasFolderActors(const Folder& FolderRecord) const;
    bool HasActorChildren(const AActor& Actor) const;
    void DrawFolder(const Folder& FolderRecord);
    void DrawActor(AActor& Actor);
    void DrawRootActors();

    UWorld* World;
    FWorldEditorContext* EditorContext;

    ImGuiTextFilter ActorFilter;
    FGuid SelectedFolderGuid{};
};
