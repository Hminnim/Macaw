#pragma once

#include "PCH.h"

#include "Core/Base/TypeInfo.h"

// =========================================================
// [State] 양방향 상태 데이터 (TStateChannel 용)
// =========================================================
struct FMessageEditorCameraState
{
    FVector3 Position;
    FRotator Rotation;
    float FOV;
};

#define JG_DECLARE_EDITOR_MESSAGE(MessageType) \
    inline static const FTypeInfo TypeInfo{ #MessageType, nullptr, nullptr }; \
    static const FTypeInfo& StaticTypeInfo() noexcept { return TypeInfo; } \
    MessageType() = default; \
    ~MessageType() = default; \
    MessageType(const MessageType&) = default; \
    MessageType& operator=(const MessageType&) = default; \
    MessageType(MessageType&&) noexcept = default; \
    MessageType& operator=(MessageType&&) noexcept = default

// =========================================================
// [Event] 단방향 메시지 데이터 (FMessageChannel 용)
// =========================================================
struct FMessageSpawnPrimitive
{
    FString PrimitiveType;
    uint32 SpawnCount;

    JG_DECLARE_EDITOR_MESSAGE(FMessageSpawnPrimitive);

    FMessageSpawnPrimitive(FString InputType, uint32 InputCount) noexcept
        : PrimitiveType(std::move(InputType)), SpawnCount(InputCount)
    {
    }
};

struct FMessageDeletePrimitive
{
    JG_DECLARE_EDITOR_MESSAGE(FMessageDeletePrimitive);
};

struct FMessageSaveScene
{
    FString SceneName;

    JG_DECLARE_EDITOR_MESSAGE(FMessageSaveScene);

    FMessageSaveScene(FString InputSceneName) noexcept
        : SceneName(std::move(InputSceneName))
    {
    }
};

struct FMessageLoadScene
{
    FString FilePath;

    JG_DECLARE_EDITOR_MESSAGE(FMessageLoadScene);

    FMessageLoadScene(FString InputFilePath) noexcept
        : FilePath(std::move(InputFilePath))
    {
    }
};

enum class EGizmoMode : uint8
{
    Translate,
    Rotate,
    Scale
};
