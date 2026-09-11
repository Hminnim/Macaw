#pragma once

#include "Core/Base/TypeInfo.h"

struct FRenderModeRequestMessage
{
    inline static const FTypeInfo TypeInfo{
        "FRenderModeRequestMessage",
        nullptr,
        nullptr
    };

    static const FTypeInfo& StaticTypeInfo() noexcept
    {
        return TypeInfo;
    }

    size_t ModeIndex = 0;

    FRenderModeRequestMessage() = default;

    FRenderModeRequestMessage(size_t modeIndex) noexcept : ModeIndex(modeIndex) {}
};