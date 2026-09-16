#include "PCH.h"

#include "FKeyboardInput.h"
#include "FKeyboardCameraMoveRequestMessage.h"

FKeyboardInput::FKeyboardInput()
{
    ResetKeyStates();
}

void FKeyboardInput::InitializeWorldCommandSender(FMessageChannel::FSender&& InSender)
{
    WorldCommandSender.emplace(std::move(InSender));
}

void FKeyboardInput::ProcessWindowMessage(UINT Message,WPARAM WParam,LPARAM LParam)
{
    UNREFERENCED_PARAMETER(LParam);

    if (Message == WM_KILLFOCUS)
    {
        ResetKeyStates();
        return;
    }

    if (WParam >= KeyCount)
    {
        return;
    }

    EKeyState& State = KeyStates[static_cast<std::uint8_t>(WParam)];

    switch (Message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (State == EKeyState::None ||
            State == EKeyState::Released)
        {
            State = EKeyState::Pressed;
        }
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (State != EKeyState::None)
        {
            State = EKeyState::Released;
        }
        break;

    default:
        break;
    }
}

EKeyState FKeyboardInput::GetKeyState(std::uint8_t VirtualKey) const
{
    return KeyStates[VirtualKey];
}

bool FKeyboardInput::IsHeld(std::uint8_t VirtualKey) const
{
    const EKeyState State = GetKeyState(VirtualKey);

    return State == EKeyState::Pressed || State == EKeyState::Down;
}

void FKeyboardInput::AdvanceKeyStates()
{
    for (EKeyState& State : KeyStates)
    {
        if (State == EKeyState::Pressed)
        {
            State = EKeyState::Down;
        }
        else if (State == EKeyState::Released)
        {
            State = EKeyState::None;
        }
    }
}

void FKeyboardInput::ResetKeyStates()
{
    KeyStates.fill(EKeyState::None);
}

void FKeyboardInput::DispatchPendingWorldCommands(
    float DeltaTime,
    bool bKeyboardCapturedByUI)
{
    if (!bKeyboardCapturedByUI &&
        WorldCommandSender.has_value())
    {
        const float ForwardAxis =
            (IsHeld('W') ? 1.0f : 0.0f) -
            (IsHeld('S') ? 1.0f : 0.0f);

        const float RightAxis =
            (IsHeld('D') ? 1.0f : 0.0f) -
            (IsHeld('A') ? 1.0f : 0.0f);

        if (ForwardAxis != 0.0f || RightAxis != 0.0f)
        {
            WorldCommandSender->TryEmplace<
                FKeyboardCameraMoveRequestMessage>(
                    ForwardAxis,
                    RightAxis,
                    DeltaTime);
        }
    }

    AdvanceKeyStates();
}