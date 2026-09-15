#include "PCH.h"
#include "FWorldEditorContext.h"

#include "AActor.h"
#include "Component/UActorComponent.h"
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

const FCameraSnapshot* FWorldEditorContext::GetCameraState() const noexcept {
    const auto Reader = SharedState.GetReader();
    return Reader.Peek().Camera ? &*Reader.Peek().Camera : nullptr;
}

void FWorldEditorContext::PublishCameraState(const FCameraSnapshot& State) {
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

const size_t FWorldEditorContext::GetRenderModeState() const noexcept
{
    return SharedState.GetReader().Peek().ModeIndex;
}

void FWorldEditorContext::SetRenderModeState(const size_t State)
{
    SharedState.GetWriter().Modify([&State](FWorldEditorSharedState& Shared) {Shared.ModeIndex = State;});
}

void FWorldEditorContext::SetSelectedCollider(UCollisionComponent* Collider) {
    if (Collider == nullptr || Collider->GetOwner() == nullptr) {
        ClearSelection();
        return;
    }
    SetSelectedComponent(Collider);
}

void FWorldEditorContext::SetSelectedActor(AActor* Actor) {
    if (Actor == nullptr) {
        ClearSelection();
        return;
    }

    SelectedCollider.Reset();
    SelectedActor.Set(Actor);
    SelectedComponent.Set(Actor->GetRootComponent());
}

void FWorldEditorContext::SetSelectedComponent(UActorComponent* Component) {
    if (Component == nullptr || Component->GetOwner() == nullptr) {
        ClearSelection();
        return;
    }

    SelectedActor.Set(Component->GetOwner());
    SelectedComponent.Set(Component);

    if (Component->GetTypeInfo()->IsA(UCollisionComponent::StaticTypeInfo())) {
        SelectedCollider.Set(static_cast<UCollisionComponent*>(Component));
    }
    else {
        SelectedCollider.Reset();
    }
}

void FWorldEditorContext::ClearSelection() {
    SelectedCollider.Reset();
    SelectedComponent.Reset();
    SelectedActor.Reset();
}

AActor* FWorldEditorContext::GetSelectedActor() const noexcept { return SelectedActor.Get(); }
UActorComponent* FWorldEditorContext::GetSelectedComponent() const noexcept { return SelectedComponent.Get(); }
UCollisionComponent* FWorldEditorContext::GetSelectedCollider() const noexcept { return SelectedCollider.Get(); }

USceneComponent* FWorldEditorContext::GetSelectedTransformTarget() const noexcept {

    if (SelectedCollider.Get() != nullptr) {
        AActor* Actor = SelectedActor.Get();
        return Actor != nullptr ? Actor->GetRootComponent() : nullptr;
    }

    UActorComponent* Component = SelectedComponent.Get();
    if (Component != nullptr && Component->GetTypeInfo()->IsA(USceneComponent::StaticTypeInfo())) {
        return static_cast<USceneComponent*>(Component);
    }

    AActor* Actor = SelectedActor.Get();
    return Actor != nullptr ? Actor->GetRootComponent() : nullptr;
}
