#include "PCH.h"
#include "FWorldEditorContext.h"

#include "AActor.h"
#include "Component/UCollisionComponent.h"
#include "Component/USceneComponent.h"
#include "Core/Asset/FAssetRegistry.h"
#include "UWorld.h"

void FWorldEditorContext::SetWorld(UWorld* InWorld) {
    World = InWorld;
}

void FWorldEditorContext::InitializeChannels(FAssetRegistry& AssetRegistry, ID3D11Device* Device) {
    if (World == nullptr) return;

    EditorToWorld.TryBind<FMessageSpawnPrimitive>([this, &AssetRegistry](const FMessageSpawnPrimitive& Message) {
        World->HandleSpawnPrimitive(Message, AssetRegistry);
    });
    EditorToWorld.TryBind<FMessageDeletePrimitive>([this](const FMessageDeletePrimitive&) {
        if (UCollisionComponent* Collider = GetSelectedCollider()) {
            World->DestroyActor(Collider->GetOwner());
            World->FlushPendingDestroyActors();
        }
    });
    EditorToWorld.TryBind<FMessageSaveScene>([this, &AssetRegistry](const FMessageSaveScene& Message) {
        World->SaveScene(Message.SceneName, &AssetRegistry);
    });
    EditorToWorld.TryBind<FMessageLoadScene>([this, &AssetRegistry, Device](const FMessageLoadScene& Message) {
        World->LoadScene(std::filesystem::path(Message.FilePath.c_str()), Device, &AssetRegistry);
    });
}

void FWorldEditorContext::Dispatch() {
    EditorToWorld.Dispatch();
    WorldToEditor.Dispatch();
}

FMessageChannel::FSender FWorldEditorContext::GetEditorToWorldSender() { return EditorToWorld.GetSender(); }
FMessageChannel::FSender FWorldEditorContext::GetWorldToEditorSender() { return WorldToEditor.GetSender(); }

const FMessageEditorCameraState* FWorldEditorContext::GetCameraState() const noexcept {
    const auto Reader = SharedState.GetReader();
    return Reader.Peek().Camera ? &*Reader.Peek().Camera : nullptr;
}

void FWorldEditorContext::SetCameraState(const FMessageEditorCameraState& State) {
    SharedState.GetWriter().Modify([&State](FWorldEditorSharedState& Shared) { Shared.Camera = State; });
}

const float FWorldEditorContext::GetGridSizeState() const noexcept
{
    return SharedState.GetReader().Peek().GridSize;
}

void FWorldEditorContext::SetGridSizeState(const float State)
{
    SharedState.GetWriter().Modify([&State](FWorldEditorSharedState& Shared) {Shared.GridSize = State;});
}

void FWorldEditorContext::SetSelectedCollider(UCollisionComponent* Collider) {
    if (Collider == nullptr || Collider->GetOwner() == nullptr) {
        ClearSelection();
        return;
    }
    SelectedCollider.Set(Collider);
    SelectedActor.Set(Collider->GetOwner());
}

void FWorldEditorContext::ClearSelection() {
    SelectedCollider.Reset();
    SelectedActor.Reset();
}

AActor* FWorldEditorContext::GetSelectedActor() const noexcept { return SelectedActor.Get(); }
UCollisionComponent* FWorldEditorContext::GetSelectedCollider() const noexcept { return SelectedCollider.Get(); }

USceneComponent* FWorldEditorContext::GetSelectedTransformTarget() const noexcept {
    AActor* Actor = SelectedActor.Get();
    return Actor != nullptr ? Actor->GetRootComponent() : nullptr;
}
