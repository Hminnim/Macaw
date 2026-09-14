#pragma once

#include <d3d11.h>
#include <optional>

#include "Core/Base/TObjectRef.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "Render/Panel/FEditorInfo.h"

class AActor;
class FAssetRegistry;
class UCollisionComponent;
class USceneComponent;
class UWorld;

struct FWorldEditorSharedState {
    std::optional<FMessageEditorCameraState> Camera;
    float GridSize{1.0f};
};

class FWorldEditorContext {
public:
    void SetWorld(UWorld* InWorld);
    void InitializeChannels(FAssetRegistry& AssetRegistry, ID3D11Device* Device);
    void Dispatch();

    FMessageChannel::FSender GetEditorToWorldSender();
    FMessageChannel::FSender GetWorldToEditorSender();

    const FMessageEditorCameraState* GetCameraState() const noexcept;
    void SetCameraState(const FMessageEditorCameraState& State);

    const float GetGridSizeState() const noexcept;
    void SetGridSizeState(const float State);

    void SetSelectedCollider(UCollisionComponent* Collider);
    void ClearSelection();

    AActor* GetSelectedActor() const noexcept;
    UCollisionComponent* GetSelectedCollider() const noexcept;
    USceneComponent* GetSelectedTransformTarget() const noexcept;

private:
    UWorld* World = nullptr;
    TObjectRef<AActor> SelectedActor;
    TObjectRef<UCollisionComponent> SelectedCollider;
    FStateChannel<FWorldEditorSharedState> SharedState{ std::in_place };
    FMessageChannel EditorToWorld{ 64 };
    FMessageChannel WorldToEditor{ 64 };
};
