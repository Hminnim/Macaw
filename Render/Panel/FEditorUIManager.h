#pragma once

#include "PCH.h"
#include "IEditorPanel.h"
#include "FControlPanel.h"
#include "FPropertyPanel.h"
#include "FConsolePanel.h"
#include "FStatPanel.h"

#include "Core/Channel/FStateChannel.h"
#include "../../Scene/FWorldEditorContext.h"

class FEditorUIManager
{
public:
    void Initialize(
        UWorld& World,

        FWorldEditorContext& EditorContext,

        HWND WindowHandle,

        FStateChannel<uint8>::FReadWriter GizmoSender,
        FStateChannel<uint8>::FReadWriter GizmoCoordinateSpaceSender
    )
    {
        Panels.emplace_back(
            std::make_unique<FControlPanel>(
                EditorContext,
                WindowHandle,
                EditorContext.GetEditorToWorldSender()
            )
        );

        Panels.emplace_back(
            std::make_unique<FPropertyPanel>(
                EditorContext,
                std::move(GizmoSender),
                std::move(GizmoCoordinateSpaceSender)
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
