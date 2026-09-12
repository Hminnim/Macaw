#include "PCH.h"
#include "FWorldEditorContext.h"

#include "AActor.h"
#include "Component/UCollisionComponent.h"
#include "Component/USceneComponent.h"
#include "UWorld.h"

FWorldEditorContext::FWorldEditorContext() = default;

void FWorldEditorContext::SetWorld(UWorld* InWorld) {
    if (World == InWorld) return;
    World = InWorld;
    if (World != nullptr) MousePickRequests.TryBind<FMousePickRequestMessage>([this](const FMousePickRequestMessage& Message) { World->HandleMousePickRequest(Message); });
}

void FWorldEditorContext::BindEditorCommands(FMessageChannel& EditorCommands) {
    EditorCommands.TryBind<FMousePickRequestMessage>([this](const FMousePickRequestMessage& Message) { RequestMousePick(Message); });
}

bool FWorldEditorContext::RequestMousePick(const FMousePickRequestMessage& Message) {
    if (World == nullptr || !MousePickRequests.TryPush(Message)) return false;
    MousePickRequests.Dispatch();
    return true;
}

void FWorldEditorContext::SetSelectedCollider(UCollisionComponent* Collider, std::uint64_t TransformRevision) {
    if (Collider == nullptr || Collider->GetOwner() == nullptr) { ClearSelection(); return; }
    SelectedCollider.Set(Collider);
    SelectedActor.Set(Collider->GetOwner());
    PublishSelectionState(TransformRevision);
}

void FWorldEditorContext::ClearSelection() { SelectedCollider.Reset(); SelectedActor.Reset(); SelectionState.GetWriter().Clear(); }
void FWorldEditorContext::RefreshSelectionState(std::uint64_t TransformRevision) { PublishSelectionState(TransformRevision); }
FStateChannel<FEditorSelectionState>::FReader FWorldEditorContext::GetSelectionStateReader() const noexcept { return SelectionState.GetReader(); }
AActor* FWorldEditorContext::GetSelectedActor() const noexcept { return SelectedActor.Get(); }
UCollisionComponent* FWorldEditorContext::GetSelectedCollider() const noexcept { return SelectedCollider.Get(); }

void FWorldEditorContext::PublishSelectionState(std::uint64_t TransformRevision) {
    UCollisionComponent* Collider = SelectedCollider.Get();
    AActor* Actor = SelectedActor.Get();
    USceneComponent* Target = Actor != nullptr ? Actor->GetRootComponent() : nullptr;
    if (Collider == nullptr || Target == nullptr) { ClearSelection(); return; }
    SelectionState.GetWriter().Emplace(FEditorSelectionState{ .TransformTargetHandle = Target->GetHandle(), .PickedColliderHandle = Collider->GetHandle(), .TargetWorld = Target->GetComponentToWorld(), .ColliderWorld = Collider->GetComponentToWorld(), .BoundsCenter = Collider->GetBoundsCenter(), .BoundsExtent = Collider->GetExtent(), .BoundsOrientation = Collider->GetBoundsOrientation(), .TransformRevision = TransformRevision });
}
