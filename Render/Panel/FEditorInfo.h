#pragma once

#include "PCH.h"

#include "Core/Base/TypeInfo.h"

// =========================================================
// [State] 양방향 상태 데이터 (TStateChannel 용)
// =========================================================
struct FCameraSnapshot
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

struct FMessageSetEditorCameraRequest
{
    FVector3 Position;
    FRotator Rotation;
    float FOV;

    JG_DECLARE_EDITOR_MESSAGE(FMessageSetEditorCameraRequest);

    FMessageSetEditorCameraRequest(
        const FVector3& InPosition,
        const FRotator& InRotation,
        float InFOV) noexcept
        : Position(InPosition)
        , Rotation(InRotation)
        , FOV(InFOV)
    {
    }
};

// =========================================================
// [Event] 단방향 메시지 데이터 (FMessageChannel 용)
// =========================================================
struct FMessageSpawnComponent
{
    FString ComponentType;
    FString MeshType;
    uint32 SpawnCount;

    JG_DECLARE_EDITOR_MESSAGE(FMessageSpawnComponent);

    FMessageSpawnComponent(FString InputComponentType, FString InputMeshType, uint32 InputCount) noexcept
        : ComponentType(std::move(InputComponentType))
        , MeshType(std::move(InputMeshType))
        , SpawnCount(InputCount)
    {
    }
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

enum class EGizmoCoordinateSpace : uint8
{
    World,
    Local
};
