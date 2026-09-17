#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "IEditorPanel.h"
#include "FEditorInfo.h"
#include "Core/Channel/FMessageChannel.h"
#include "../../Scene/FWorldEditorContext.h"

class FControlPanel : public IEditorPanel
{
public:
    FControlPanel(
        FWorldEditorContext& InEditorContext,
        HWND InputWindowHandle,
        FMessageChannel::FSender InEditorToWorldSender
    )
        : EditorContext(&InEditorContext)
        , WindowHandle(InputWindowHandle)
        , EditorToWorldSender(std::move(InEditorToWorldSender))
    {
    }

    void DrawPanel() override;

    FString OpenFileDialog();

private:
    FWorldEditorContext* EditorContext = nullptr;
    FMessageChannel::FSender EditorToWorldSender;

private:
    char SceneNameBuffer[256] = "NewScene";

    int SelectedComponentIndex = -1;
    int SelectedMeshIndex = 0;
    int SpawnCountToRequest = 1;

    FVector3 CachedCamPos{};
    FRotator CachedCamRot{};

    // UCameraComponent와 동일하게 radians
    float CachedFOV = 1.0472f;

    float GridSize{};

    size_t RenderModeIndex = 0;

    // Components 체크리스트에서 컴포넌트 타입을 검색한다.
    ImGuiTextFilter ComponentFilter;

    HWND WindowHandle;
};
