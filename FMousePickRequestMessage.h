#pragma once

#include <cstdint>

#include "Core/Base/TypeInfo.h"

struct FMousePickRequestMessage
{
	inline static const FTypeInfo TypeInfo{
		"FMousePickRequestMessage",
		nullptr,
		nullptr
	};

	static const FTypeInfo& StaticTypeInfo() noexcept
	{
		return TypeInfo;
	}

	std::int32_t ScreenX = 0;
	std::int32_t ScreenY = 0;
	std::uint32_t ViewportWidth = 0;
	std::uint32_t ViewportHeight = 0;

	FMousePickRequestMessage() = default;

	FMousePickRequestMessage(
		std::int32_t InScreenX,
		std::int32_t InScreenY,
		std::uint32_t InViewportWidth,
		std::uint32_t InViewportHeight) noexcept
		: ScreenX(InScreenX)
		, ScreenY(InScreenY)
		, ViewportWidth(InViewportWidth)
		, ViewportHeight(InViewportHeight)
	{
	}
};
