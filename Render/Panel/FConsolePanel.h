#pragma once

#include "Render/Panel/IEditorPanel.h"
#include "Render/Panel/Console/ConsoleWindow.h"

class FConsolePanel : public IEditorPanel
{
public:
    explicit FConsolePanel(FConsoleOutputHandle InHandle) : Handle(InHandle)
    {
    }

    void DrawPanel() override
    {
        DrawConsole(Console::STDOutHandle);
    }

private:
    FConsoleOutputHandle Handle;
};