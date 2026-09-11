#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "IEditorPanel.h"
#include "FEditorInfo.h"
#include "Core/Channel/FStateChannel.h"
#include "Core/Channel/FMessageChannel.h"

std::string GetFilePathFromExplorer();
FString OpenFileDialog();

class FControlPanel : public IEditorPanel
{
public:
    FControlPanel(
        FStateChannel<FMessageEditorCameraState>::FWriter InCamWriter,
        FStateChannel<FMessageEditorCameraState>::FReader InCamReader,
        FStateChannel<size_t>::FWriter InRenderModeWriter,
        HWND InputWindowHandle,
        FMessageChannel::FSender InSpawnSender,
        FMessageChannel::FSender InSceneSender
    )
        : CamWriter(std::move(InCamWriter))
        , CamReader(std::move(InCamReader))
        , RenderModeWriter(std::move(InRenderModeWriter))
        , WindowHandle(InputWindowHandle)
        , SpawnSender(std::move(InSpawnSender))
        , SceneSender(std::move(InSceneSender))
    {
    }

    void DrawPanel() override;

    FString OpenFileDialog();

private:
    FStateChannel<FMessageEditorCameraState>::FWriter CamWriter;
    FStateChannel<FMessageEditorCameraState>::FReader CamReader;
    FStateChannel<size_t>::FWriter RenderModeWriter;

    //FMessageChannel::FSender WorldCommandSender;

    FMessageChannel::FSender SpawnSender;
    FMessageChannel::FSender SceneSender;

private:
    char SceneNameBuffer[256] = "NewScene";

    int SelectedPrimitiveIndex = 0;
    int SpawnCountToRequest = 1;

    FVector3 CachedCamPos{};
    FRotator CachedCamRot{};

    // UCameraComponent와 동일하게 radians
    float CachedFOV = 1.0472f;

    HWND WindowHandle;

    size_t RenderModeIndex = 0;
};