#pragma once

#include "Core/Base/TypeInfo.h"

struct FKeyboardCameraMoveRequestMessage
{
    inline static const FTypeInfo TypeInfo{
        "FKeyboardCameraMoveRequestMessage",
        nullptr,
        nullptr
    };

    static const FTypeInfo& StaticTypeInfo() noexcept
    {
        return TypeInfo;
    }

    float ForwardAxis = 0.0f;
    float RightAxis = 0.0f;
    float DeltaTime = 0.0f;

    FKeyboardCameraMoveRequestMessage() = default;

    FKeyboardCameraMoveRequestMessage(
        float InForwardAxis,
        float InRightAxis,
        float InDeltaTime) noexcept
        : ForwardAxis(InForwardAxis)
        , RightAxis(InRightAxis)
        , DeltaTime(InDeltaTime)
    {
    }
};