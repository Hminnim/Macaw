#include "PCH.h"
#include "FWorldEditorContext.h"

#include "AActor.h"
#include "Component/UCollisionComponent.h"
#include "Component/USceneComponent.h"
#include "Core/Asset/FAssetRegistry.h"
#include "FTransformEditRequestMessage.h"
#include "UWorld.h"

void FWorldEditorContext::SetWorld(UWorld* InWorld) {
    World = InWorld;
}

void FWorldEditorContext::InitializeChannels(FAssetRegistry& AssetRegistry, ID3D11Device* Device) {
    if (World == nullptr) return;

    EditorToWorld.TryBind<FTransformEditRequestMessage>([this](const FTransformEditRequestMessage& Message) {
        World->HandleTransformEditRequest(Message);
    });
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

const FEditorSelectionState* FWorldEditorContext::GetSelectionState() const noexcept {
    const auto Reader = SharedState.GetReader();
    return Reader.Peek().Selection ? &*Reader.Peek().Selection : nullptr;
}

const FMessageEditorCameraState* FWorldEditorContext::GetCameraState() const noexcept {
    const auto Reader = SharedState.GetReader();
    return Reader.Peek().Camera ? &*Reader.Peek().Camera : nullptr;
}

void FWorldEditorContext::SetCameraState(const FMessageEditorCameraState& State) {
    SharedState.GetWriter().Modify([&State](FWorldEditorSharedState& Shared) { Shared.Camera = State; });
}

void FWorldEditorContext::SetSelectedCollider(UCollisionComponent* Collider, std::uint64_t TransformRevision) {
    if (Collider == nullptr || Collider->GetOwner() == nullptr) {
        ClearSelection();
        return;
    }
    SelectedCollider.Set(Collider);
    SelectedActor.Set(Collider->GetOwner());
    PublishSelectionState(TransformRevision);
}

void FWorldEditorContext::ClearSelection() {
    SelectedCollider.Reset();
    SelectedActor.Reset();
    SharedState.GetWriter().Modify([](FWorldEditorSharedState& State) { State.Selection.reset(); });
}

void FWorldEditorContext::RefreshSelectionState(std::uint64_t TransformRevision) { PublishSelectionState(TransformRevision); }
AActor* FWorldEditorContext::GetSelectedActor() const noexcept { return SelectedActor.Get(); }
UCollisionComponent* FWorldEditorContext::GetSelectedCollider() const noexcept { return SelectedCollider.Get(); }

void FWorldEditorContext::PublishSelectionState(std::uint64_t TransformRevision) {
    UCollisionComponent* Collider = SelectedCollider.Get();
    AActor* Actor = SelectedActor.Get();
    USceneComponent* Target = Actor != nullptr ? Actor->GetRootComponent() : nullptr;
    if (Collider == nullptr || Target == nullptr) {
        ClearSelection();
        return;
    }
    SharedState.GetWriter().Modify([&](FWorldEditorSharedState& State) {
        State.Selection = FEditorSelectionState{
            .TransformTargetHandle = Target->GetHandle(),
            .PickedColliderHandle = Collider->GetHandle(),
            .TargetWorld = Target->GetComponentToWorld(),
            .ColliderWorld = Collider->GetComponentToWorld(),
            .BoundsCenter = Collider->GetBoundsCenter(),
            .BoundsExtent = Collider->GetExtent(),
            .BoundsOrientation = Collider->GetBoundsOrientation(),
            .TransformRevision = TransformRevision
        };
    });
}
