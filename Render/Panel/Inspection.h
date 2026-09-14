#pragma once

#include "ImGui/imgui.h"
#include "Render/Panel/IEditorPanel.h"
#include "Scene/UWorld.h"

    // UI-only reference for the Unreal-style Outliner. It deliberately never reads UWorld.
class FOutlinerPanel : public IEditorPanel
{
public:
    explicit FOutlinerPanel(UWorld&)
    {
    }

    void DrawPanel() override
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.080f, 0.095f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.165f, 0.215f, 0.285f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.215f, 0.310f, 0.425f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.255f, 0.385f, 0.540f, 1.0f));

        if (!ImGui::Begin("Outliner###OutlinerPanel"))
        {
            ImGui::End();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);
            return;
        }

        ImGui::SetNextItemWidth(-1.0f);
        ActorFilter.Draw("Search");
        ImGui::Checkbox("Type", &bShowTypeColumn);
        ImGui::SameLine();
        ImGui::TextDisabled("Sample Level");
        ImGui::Separator();

        const int ColumnCount = bShowTypeColumn ? 3 : 2;
        if (ImGui::BeginTable("OutlinerActorList", ColumnCount,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
            ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing())))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.70f);
            if (bShowTypeColumn)
            {
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.24f);
            }
            ImGui::TableSetupColumn("Visibility", ImGuiTableColumnFlags_WidthFixed, 22.0f);
            ImGui::TableHeadersRow();

            for (const FOutlinerItem& Item : RootItems)
            {
                DrawOutlinerItem(Item);
            }

            ImGui::EndTable();
        }

        ImGui::TextDisabled("10 Actors | 3 Folders | Sample data only");
        ImGui::End();

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
    }

private:
    enum class EOutlinerItemKind
    {
        Folder,
        Actor,
    };

    struct FOutlinerItem
    {
        const char* Id;
        const char* Label;
        const char* Type;
        EOutlinerItemKind Kind;
        const FOutlinerItem* Children;
        int ChildCount;
    };

    bool MatchesFilter(const FOutlinerItem& Item) const
    {
        if (ActorFilter.PassFilter(Item.Label) || (Item.Type != nullptr && ActorFilter.PassFilter(Item.Type)))
        {
            return true;
        }

        for (int ChildIndex = 0; ChildIndex < Item.ChildCount; ++ChildIndex)
        {
            if (MatchesFilter(Item.Children[ChildIndex]))
            {
                return true;
            }
        }

        return false;
    }

    void DrawOutlinerItem(const FOutlinerItem& Item)
    {
        if (!MatchesFilter(Item))
        {
            return;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAllColumns;
        if (Item.ChildCount == 0)
        {
            Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (Item.Kind == EOutlinerItemKind::Folder)
        {
            Flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        if (SelectedItemId == Item.Id)
        {
            Flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool bOpen = ImGui::TreeNodeEx(Item.Id, Flags, "%s", Item.Label);
        if (ImGui::IsItemClicked())
        {
            SelectedItemId = Item.Id;
        }

        if (bShowTypeColumn)
        {
            ImGui::TableSetColumnIndex(1);
            if (Item.Kind == EOutlinerItemKind::Actor)
            {
                ImGui::TextDisabled("%s", Item.Type);
            }
        }

        ImGui::TableSetColumnIndex(bShowTypeColumn ? 2 : 1);
        ImGui::TextDisabled(Item.Kind == EOutlinerItemKind::Actor ? "o" : "");

        if (Item.ChildCount != 0 && bOpen)
        {
            for (int ChildIndex = 0; ChildIndex < Item.ChildCount; ++ChildIndex)
            {
                DrawOutlinerItem(Item.Children[ChildIndex]);
            }
            ImGui::TreePop();
        }
    }

private:
    // Folders are organizational only. Indented Actor entries represent attachments.
    inline static const FOutlinerItem CityBlockChildren[] =
    {
        { "OfficeTower", "SM_OfficeTower_A", "StaticMeshActor", EOutlinerItemKind::Actor, nullptr, 0 },
        { "StreetLamp", "BP_StreetLamp_03", "Blueprint", EOutlinerItemKind::Actor, nullptr, 0 },
    };

    inline static const FOutlinerItem EnvironmentChildren[] =
    {
        { "CityBlock", "BP_CityBlock", "Blueprint", EOutlinerItemKind::Actor, CityBlockChildren, IM_ARRAYSIZE(CityBlockChildren) },
        { "SkySphere", "BP_SkySphere", "Blueprint", EOutlinerItemKind::Actor, nullptr, 0 },
    };

    inline static const FOutlinerItem CharacterChildren[] =
    {
        { "Weapon", "BP_Weapon_Rifle", "Blueprint", EOutlinerItemKind::Actor, nullptr, 0 },
        { "Drone", "BP_CompanionDrone", "Blueprint", EOutlinerItemKind::Actor, nullptr, 0 },
    };

    inline static const FOutlinerItem GameplayChildren[] =
    {
        { "Character", "BP_ThirdPersonCharacter", "Character", EOutlinerItemKind::Actor, CharacterChildren, IM_ARRAYSIZE(CharacterChildren) },
        { "PlayerStart", "PlayerStart", "PlayerStart", EOutlinerItemKind::Actor, nullptr, 0 },
    };

    inline static const FOutlinerItem LightingChildren[] =
    {
        { "Sun", "DirectionalLight", "DirectionalLight", EOutlinerItemKind::Actor, nullptr, 0 },
        { "SkyLight", "SkyLight", "SkyLight", EOutlinerItemKind::Actor, nullptr, 0 },
    };

    inline static const FOutlinerItem RootItems[] =
    {
        { "Environment", "Environment", nullptr, EOutlinerItemKind::Folder, EnvironmentChildren, IM_ARRAYSIZE(EnvironmentChildren) },
        { "Gameplay", "Gameplay", nullptr, EOutlinerItemKind::Folder, GameplayChildren, IM_ARRAYSIZE(GameplayChildren) },
        { "Lighting", "Lighting", nullptr, EOutlinerItemKind::Folder, LightingChildren, IM_ARRAYSIZE(LightingChildren) },
    };

    ImGuiTextFilter ActorFilter;
    const char* SelectedItemId = "Character";
    bool bShowTypeColumn = false;
};
