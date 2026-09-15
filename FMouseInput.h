#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <Windows.h>

#include "Core/Channel/FMessageChannel.h"
#include "EKeyState.h"


enum EMouseSide : uint32 {
    Left, Right, MAX
};

class FMouseInput
{
public:
	void InitializeWorldCommandSender(FMessageChannel::FSender&& InSender);

	void ProcessWindowMessage(UINT Message, WPARAM WParam, LPARAM LParam);

	void DispatchPendingWorldCommands(std::uint32_t ViewportWidth, std::uint32_t ViewportHeight, bool bMouseCaptureByUI);

    EKeyState GetKeyState(EMouseSide Side) const;

    struct DragCapture
    {
        POINT start{};
        POINT current{};
    };

    const DragCapture& GetDragCapture(EMouseSide Side) const;

    void Consume(EMouseSide Side);
    void EndFrame();

private:
    enum class EDragOwner : std::uint8_t
    {
        None,
        World,
        UI,
        Gizmo
    };

    void ResetKeyStates();

    std::optional<FMessageChannel::FSender> WorldCommandSender;

    std::array<EKeyState, MAX> KeyStates{
        EKeyState::None,
        EKeyState::None
    };

    std::array<DragCapture, MAX> ClickCaptures{};

    std::array<EDragOwner, MAX> DragOwners{
        EDragOwner::None,
        EDragOwner::None
    };

    float PendingRotateDeltaX = 0.0f;
    float PendingRotateDeltaY = 0.0f;
};
