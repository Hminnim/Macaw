#include "PCH.h"

#include "Outliner.h"
#include "../../Scene/Component/UNameTagComponent.h"
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

    const ImGuiTableFlags TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("OutlinerActorList", 2, TableFlags, ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()))) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.70f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.24f);
        ImGui::TableHeadersRow();
        DrawRootActors();
        ImGui::EndTable();
    }

    HandleDeleteShortcut();
    ImGui::TextDisabled("%zu Actors", World->GetActors().size());
    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
}

bool FOutlinerPanel::MatchesActor(const AActor& Actor) const {
    const FString Label = Actor.GetGuid().ToString();
    const std::string_view TypeName = Actor.GetTypeInfo()->TypeName;
    if (ActorFilter.PassFilter(Label.c_str()) || ActorFilter.PassFilter(TypeName.data(), TypeName.data() + TypeName.size())) {
        return true;
    }

    return std::ranges::any_of(World->GetActors(), [this, &Actor](const std::unique_ptr<AActor>& ChildActor) {
        return IsActorAttachedTo(*ChildActor, Actor) && MatchesActor(*ChildActor);
    });
}

bool FOutlinerPanel::IsActorAttachedTo(const AActor& Actor, const AActor& ParentActor) const {
    const USceneComponent* RootComponent = Actor.GetRootComponent();
    const USceneComponent* ParentComponent = RootComponent != nullptr ? RootComponent->GetParent() : nullptr;
    return ParentComponent != nullptr && ParentComponent->GetOwner() == &ParentActor && &Actor != &ParentActor;
}

bool FOutlinerPanel::IsRootActor(const AActor& Actor) const {
    const USceneComponent* RootComponent = Actor.GetRootComponent();
    const USceneComponent* ParentComponent = RootComponent != nullptr ? RootComponent->GetParent() : nullptr;
    const AActor* ParentActor = ParentComponent != nullptr ? ParentComponent->GetOwner() : nullptr;
    return ParentActor == nullptr || ParentActor == &Actor;
}

bool FOutlinerPanel::HasActorChildren(const AActor& Actor) const {
    return std::ranges::any_of(World->GetActors(), [this, &Actor](const std::unique_ptr<AActor>& ChildActor) {
        return IsActorAttachedTo(*ChildActor, Actor);
    });
}

void FOutlinerPanel::HandleDeleteShortcut() {
    const ImGuiIO& IO = ImGui::GetIO();
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || IO.WantTextInput || ImGui::IsAnyItemActive() || !ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        return;
    }

    AActor* Actor = EditorContext->GetSelectedActor();
    if (Actor != nullptr && World->DestroyActor(Actor)) {
        World->FlushPendingDestroyActors();
    }
}

void FOutlinerPanel::DrawActor(AActor& Actor) {
    if (!MatchesActor(Actor)) {
        return;
    }

    const FString Label = Actor.GetName().ToString();
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

		auto prev = EditorContext->GetSelectedActor();
        
        if (prev != nullptr) {
            if (UNameTagComponent* NameTag = prev->GetComponent<UNameTagComponent>()) {
                NameTag->SetActive(false);
            }
        }

        EditorContext->SetSelectedActor(&Actor);


        if (UNameTagComponent* NameTag = Actor.GetComponent<UNameTagComponent>()) {
            NameTag->SetActive(true);
        }

    }

    ImGui::TableSetColumnIndex(1);
    ImGui::TextDisabled("%.*s", static_cast<int>(TypeName.size()), TypeName.data());

    if (bHasChildren && bOpen) {
        for (const std::unique_ptr<AActor>& ChildActor : World->GetActors()) {
            if (IsActorAttachedTo(*ChildActor, Actor)) {
                DrawActor(*ChildActor);
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FOutlinerPanel::DrawRootActors() {
    for (const std::unique_ptr<AActor>& Actor : World->GetActors()) {
        if (IsRootActor(*Actor)) {
            DrawActor(*Actor);
        }
    }
}
