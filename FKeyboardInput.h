#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <Windows.h>

#include "Core/Channel/FMessageChannel.h"
#include "EKeyState.h"

class FKeyboardInput
{
public:
    FKeyboardInput();

    void InitializeWorldCommandSender(FMessageChannel::FSender&& Sender);

    void ProcessWindowMessage(UINT Message, WPARAM WParam, LPARAM LParam);

    void DispatchPendingWorldCommands(float DeltaTime, bool bKeyboardCaptureByUI);

    EKeyState GetKeyState(std::uint8_t VirtualKey) const;

private:
    static constexpr std::size_t KeyCount = 256;

    bool IsHeld(std::uint8_t VirtualKey) const;
    void AdvanceKeyStates();
    void ResetKeyStates();

    std::array<EKeyState, KeyCount> KeyStates;
    std::optional<FMessageChannel::FSender> WorldCommandSender;

};

