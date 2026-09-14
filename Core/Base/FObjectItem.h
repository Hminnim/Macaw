#pragma once

#include <cstdint>

#include "UObject.h"

struct FObjectItem
{
	UObject* Object = nullptr;
	std::uint32_t Generation = 1;
};
