#include "PCH.h"

#include "Outliner.h"

#include "Scene/FWorldEditorContext.h"

#include <ranges>

FOutlinerPanel::FOutlinerPanel(UWorld& InWorld, FWorldEditorContext& InEditorContext)
    : World(&InWorld)
    , EditorContext(&InEditorContext) {
}

void FOutlinerPanel::DrawPanel() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.080f, 0.095f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.165f, 0.215f, 0.285f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.215f, 0.310f, 0.425f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.255f, 0.385f, 0.540f, 1.0f));

    if (!ImGui::Begin("Outliner###OutlinerPanel")) {
        ImGui::End();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
        return;
    }

    ImGui::SetNextItemWidth(-FLT_MIN);

    if (ImGui::InputTextWithHint("##ActorFilter", "Search", ActorFilter.InputBuf, IM_ARRAYSIZE(ActorFilter.InputBuf))) {
        ActorFilter.Build();
    }

    const int ColumnCount = 2;
    const ImGuiTableFlags TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("OutlinerActorList", ColumnCount, TableFlags, ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()))) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.70f);

        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.24f);
        
        ImGui::TableHeadersRow();

        for (const std::unique_ptr<Folder>& FolderRecord : World->GetFolders()) {
            if (FolderRecord->IsRootFolder()) {
                DrawFolder(*FolderRecord);
            }
        }
        DrawRootActors();
        ImGui::EndTable();
    }

    ImGui::TextDisabled("%zu Actors | %zu Folders", World->GetActors().size(), World->GetFolders().size());
    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
}

bool FOutlinerPanel::MatchesFolder(const Folder& FolderRecord) const {
    const FString Label = FolderRecord.GetID().ToString();
    if (ActorFilter.PassFilter(Label.c_str())) {
        return true;
    }

    for (const std::unique_ptr<Folder>& ChildFolder : World->GetFolders()) {
        if (ChildFolder->GetParentFolderGuid() == FolderRecord.GetID() && MatchesFolder(*ChildFolder)) {
            return true;
        }
    }

    for (const std::unique_ptr<AActor>& Actor : World->GetActors()) {
        if (Actor->GetFolderGuid() == FolderRecord.GetID() && IsActorRootInFolder(*Actor) && MatchesActor(*Actor)) {
            return true;
        }
    }

    return false;
}

bool FOutlinerPanel::MatchesActor(const AActor& Actor) const {
    const FString Label = Actor.GetGuid().ToString();
    const std::string_view TypeName = Actor.GetTypeInfo()->TypeName;
    if (ActorFilter.PassFilter(Label.c_str()) || ActorFilter.PassFilter(TypeName.data(), TypeName.data() + TypeName.size())) {
        return true;
    }

    for (const std::unique_ptr<AActor>& ChildActor : World->GetActors()) {
        if (ChildActor->GetFolderGuid() == Actor.GetFolderGuid() && IsActorAttachedTo(*ChildActor, Actor) && MatchesActor(*ChildActor)) {
            return true;
        }
    }

    return false;
}

bool FOutlinerPanel::IsActorAttachedTo(const AActor& Actor, const AActor& ParentActor) const {
    const USceneComponent* RootComponent = Actor.GetRootComponent();
    const USceneComponent* ParentComponent = RootComponent != nullptr ? RootComponent->GetParent() : nullptr;
    return ParentComponent != nullptr && ParentComponent->GetOwner() == &ParentActor && &Actor != &ParentActor;
}

bool FOutlinerPanel::IsActorRootInFolder(const AActor& Actor) const {
    const USceneComponent* RootComponent = Actor.GetRootComponent();
    const USceneComponent* ParentComponent = RootComponent != nullptr ? RootComponent->GetParent() : nullptr;
    const AActor* ParentActor = ParentComponent != nullptr ? ParentComponent->GetOwner() : nullptr;
    return ParentActor == nullptr || ParentActor == &Actor || ParentActor->GetFolderGuid() != Actor.GetFolderGuid();
}

bool FOutlinerPanel::HasChildFolders(const Folder& FolderRecord) const {
    return std::ranges::any_of(World->GetFolders(), [&FolderRecord](const std::unique_ptr<Folder>& ChildFolder) {
        return ChildFolder->GetParentFolderGuid() == FolderRecord.GetID();
    });
}

bool FOutlinerPanel::HasFolderActors(const Folder& FolderRecord) const {
    return std::ranges::any_of(World->GetActors(), [this, &FolderRecord](const std::unique_ptr<AActor>& Actor) {
        return Actor->GetFolderGuid() == FolderRecord.GetID() && IsActorRootInFolder(*Actor);
    });
}

bool FOutlinerPanel::HasActorChildren(const AActor& Actor) const {
    return std::ranges::any_of(World->GetActors(), [this, &Actor](const std::unique_ptr<AActor>& ChildActor) {
        return ChildActor->GetFolderGuid() == Actor.GetFolderGuid() && IsActorAttachedTo(*ChildActor, Actor);
    });
}

void FOutlinerPanel::DrawFolder(const Folder& FolderRecord) {
    if (!MatchesFolder(FolderRecord)) {
        return;
    }

    const FString Label = FolderRecord.GetID().ToString();
    const bool bHasChildren = HasChildFolders(FolderRecord) || HasFolderActors(FolderRecord);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::PushID(Label.c_str());

    ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen;
    if (!bHasChildren) {
        Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (SelectedFolderGuid == FolderRecord.GetID()) {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool bOpen = ImGui::TreeNodeEx("Folder", Flags, "%s", Label.c_str());
    if (ImGui::IsItemClicked()) {
        SelectedFolderGuid = FolderRecord.GetID();
        EditorContext->ClearSelection();
    }

    ImGui::TableSetColumnIndex(2);
    ImGui::TextDisabled("-");

    if (bHasChildren && bOpen) {
        for (const std::unique_ptr<Folder>& ChildFolder : World->GetFolders()) {
            if (ChildFolder->GetParentFolderGuid() == FolderRecord.GetID()) {
                DrawFolder(*ChildFolder);
            }
        }
        for (const std::unique_ptr<AActor>& Actor : World->GetActors()) {
            if (Actor->GetFolderGuid() == FolderRecord.GetID() && IsActorRootInFolder(*Actor)) {
                DrawActor(*Actor);
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FOutlinerPanel::DrawActor(AActor& Actor) {
    if (!MatchesActor(Actor)) {
        return;
    }

    const FString Label = Actor.GetGuid().ToString(); // FNAME
    const std::string_view TypeName = Actor.GetTypeInfo()->TypeName;
    const bool bHasChildren = HasActorChildren(Actor);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::PushID(Label.c_str());

    ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAllColumns;
    if (!bHasChildren) {
        Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (EditorContext->GetSelectedActor() == &Actor) {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool bOpen = ImGui::TreeNodeEx("Actor", Flags, "%s", Label.c_str());
    if (ImGui::IsItemClicked()) {
        SelectedFolderGuid = {};
        EditorContext->SetSelectedActor(&Actor);
    }

  
    ImGui::TableSetColumnIndex(1);
    ImGui::TextDisabled("%.*s", static_cast<int>(TypeName.size()), TypeName.data()); // Type 
    

    if (bHasChildren && bOpen) {
        for (const std::unique_ptr<AActor>& ChildActor : World->GetActors()) {
            if (ChildActor->GetFolderGuid() == Actor.GetFolderGuid() && IsActorAttachedTo(*ChildActor, Actor)) {
                DrawActor(*ChildActor);
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FOutlinerPanel::DrawRootActors() {
    for (const std::unique_ptr<AActor>& Actor : World->GetActors()) {
        if ((!Actor->GetFolderGuid().IsValid() || World->FindFolder(Actor->GetFolderGuid()) == nullptr) && IsActorRootInFolder(*Actor)) {
            DrawActor(*Actor);
        }
    }
}
