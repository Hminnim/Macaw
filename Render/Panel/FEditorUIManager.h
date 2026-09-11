#pragma once

#include "PCH.h"
#include "IEditorPanel.h"
#include "FControlPanel.h"
#include "FPropertyPanel.h"
#include "FConsolePanel.h"
#include "FStatPanel.h"

#include "Core/Channel/FStateChannel.h"
#include "Core/Channel/FMessageChannel.h"
#include "../../FEditorSelectionState.h"

class FEditorUIManager
{
public:
    void Initialize(
        UWorld& World,

        FStateChannel<FMessageEditorCameraState>::FWriter CamWriter,
        FStateChannel<FMessageEditorCameraState>::FReader CamReader,
        FStateChannel<size_t>::FWriter ModeWriter,

        HWND WindowHandle,

        FStateChannel<FEditorSelectionState>::FReader SelectionReader,
        FMessageChannel::FSender WorldCommandSender,

        FMessageChannel::FSender SpawnSender,
        FMessageChannel::FSender SceneSender,
        FStateChannel<uint8>::FReadWriter GizmoSender
    )
    {
        Panels.emplace_back(
            std::make_unique<FControlPanel>(
                std::move(CamWriter),
                std::move(CamReader),
                std::move(ModeWriter),
                WindowHandle,
                std::move(SpawnSender),
                std::move(SceneSender)
            )
        );

        Panels.emplace_back(
            std::make_unique<FPropertyPanel>(
                std::move(SelectionReader),
                std::move(GizmoSender),
                std::move(WorldCommandSender)
            )
        );

        Panels.emplace_back(
            std::make_unique<FConsolePanel>(
                Console::STDOutHandle
            )
        );

        Panels.emplace_back(
            std::make_unique<FStatPanel>(World)
        );
    }

    void Tick()
    {
        for (const std::unique_ptr<IEditorPanel>& Panel : Panels)
        {
            if (Panel != nullptr &&
                Panel->IsVisible())
            {
                Panel->DrawPanel();
            }
        }
    }

private:
    std::vector<std::unique_ptr<IEditorPanel>> Panels;
};
