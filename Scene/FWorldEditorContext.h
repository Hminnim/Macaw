#pragma once

#include <d3d11.h>
#include <optional>

#include "Core/Base/TObjectRef.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "Render/Panel/FEditorInfo.h"

class AActor;
class FAssetRegistry;
class UActorComponent;
class UCollisionComponent;
class USceneComponent;
class UWorld;

struct FWorldEditorSharedState {
    std::optional<FCameraSnapshot> Camera;
    size_t ModeIndex{ 0 };
};

class FWorldEditorContext {
public:
    void SetWorld(UWorld* InWorld);
    void InitializeChannels(FAssetRegistry& AssetRegistry, ID3D11Device* Device);
    void Dispatch();

    FMessageChannel::FSender GetEditorToWorldSender();
    FMessageChannel::FSender GetWorldToEditorSender();

    const FCameraSnapshot* GetCameraState() const noexcept;
    void PublishCameraState(const FCameraSnapshot& State);

    const size_t GetRenderModeState() const noexcept;
    void SetRenderModeState(const size_t State);

    void SetSelectedCollider(UCollisionComponent* Collider);
    void SetSelectedActor(AActor* Actor);
    void SetSelectedComponent(UActorComponent* Component);
    void ClearSelection();

    AActor* GetSelectedActor() const noexcept;
    UActorComponent* GetSelectedComponent() const noexcept;
    UCollisionComponent* GetSelectedCollider() const noexcept;
    USceneComponent* GetSelectedTransformTarget() const noexcept;

    UWorld* GetWorld() const { return World; }

private:
    UWorld* World = nullptr;
    TObjectRef<AActor> SelectedActor;
    TObjectRef<UActorComponent> SelectedComponent;
    TObjectRef<UCollisionComponent> SelectedCollider;
    FStateChannel<FWorldEditorSharedState> SharedState{ std::in_place };
    FMessageChannel EditorToWorld{ 64 };
    FMessageChannel WorldToEditor{ 64 };
};
