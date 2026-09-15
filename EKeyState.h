#pragma once

#include <cstdint>

enum class EKeyState : std::uint8_t
{
	Pressed,
	Down,
	Released,
	None
};