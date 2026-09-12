#pragma once

#include <d3d11.h>
#include <optional>

#include "Core/Base/TObjectRef.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "FEditorSelectionState.h"
#include "Render/Panel/FEditorInfo.h"

class AActor;
class FAssetRegistry;
class UCollisionComponent;
class UWorld;

struct FWorldEditorSharedState {
    std::optional<FEditorSelectionState> Selection;
    std::optional<FMessageEditorCameraState> Camera;
};

class FWorldEditorContext {
public:
    void SetWorld(UWorld* InWorld);
    void InitializeChannels(FAssetRegistry& AssetRegistry, ID3D11Device* Device);
    void Dispatch();

    FMessageChannel::FSender GetEditorToWorldSender();
    FMessageChannel::FSender GetWorldToEditorSender();

    const FEditorSelectionState* GetSelectionState() const noexcept;
    const FMessageEditorCameraState* GetCameraState() const noexcept;
    void SetCameraState(const FMessageEditorCameraState& State);

    void SetSelectedCollider(UCollisionComponent* Collider, std::uint64_t TransformRevision);
    void ClearSelection();
    void RefreshSelectionState(std::uint64_t TransformRevision);

    AActor* GetSelectedActor() const noexcept;
    UCollisionComponent* GetSelectedCollider() const noexcept;

private:
    void PublishSelectionState(std::uint64_t TransformRevision);

    UWorld* World = nullptr;
    TObjectRef<AActor> SelectedActor;
    TObjectRef<UCollisionComponent> SelectedCollider;
    FStateChannel<FWorldEditorSharedState> SharedState{ std::in_place };
    FMessageChannel EditorToWorld{ 64 };
    FMessageChannel WorldToEditor{ 64 };
};
