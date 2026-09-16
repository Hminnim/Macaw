#include "PCH.h"
#include "UActorComponent.h"
#include "../AActor.h"
#include "Render/Panel/FPropertyEditorContext.h"
#include "../../ErrorHandler.h"

AActor* UActorComponent::GetOwner() const {
    return Owner;
}

void UActorComponent::SetOwner(AActor* InOwner) {
    Owner = InOwner;
}

void UActorComponent::OnRegister() {
}

void UActorComponent::InitializeComponent() {
    bInitialized = true;
}

void UActorComponent::BeginPlay() {
    bHasBegunPlay = true;
}

void UActorComponent::EndPlay() {
    bHasBegunPlay = false;
}

void UActorComponent::Tick(float /*DeltaTime*/) {
}

void UActorComponent::OnUnregister() {
}

void UActorComponent::DrawPanels(FPropertyEditorContext& Context) {
    Context.DrawBool("Active", IsActive(), [this](bool bActive) {
        SetActive(bActive);
    });
}

bool UActorComponent::IsActive() const {
    return bActive;
}

void UActorComponent::SetActive(bool bInActive) {
    bActive = bInActive;
}

bool UActorComponent::IsRegistered() const {
    return bRegistered;
}

bool UActorComponent::IsInitialized() const {
    return bInitialized;
}

bool UActorComponent::HasBegunPlay() const {
    return bHasBegunPlay;
}

UWorld* UActorComponent::GetBelongingWorld() const {
    return ParentWorld;
}

void UActorComponent::RegisterComponent(UWorld* world) {
    ErrorHandler::Report(Owner == nullptr and ParentWorld == nullptr, "[ UActorComponent ]", "Owner and ParentWorld must not be null.", ErrorHandler::EErrorLevel::Critical);
    ErrorHandler::Report(world != Owner->GetWorld(), "[ UActorComponent ]", "World must match Owner's world.", ErrorHandler::EErrorLevel::Critical);
    if (bRegistered) return;

    ParentWorld = world;
    bRegistered = true;

    this->OnRegister();
}

void UActorComponent::UnregisterComponent() {
    if (not bRegistered) {
        return;
    }

    ErrorHandler::Report(Owner == nullptr or ParentWorld == nullptr, "[ UActorComponent ]", "Owner and ParentWorld must not be null.", ErrorHandler::EErrorLevel::Critical);

    if (bHasBegunPlay) {
        EndPlay();
    }

    this->OnUnregister();
    bRegistered = false;
    ParentWorld = nullptr;
}

void UActorComponent::DestroyComponent(bool /*bPromoteChildren*/) {
    if (bIsBeingDestroyed) {
        return;
    }

    bIsBeingDestroyed = true;
    UnregisterComponent();

    if (Owner != nullptr) {
        Owner->RemoveOwnedComponent(this);
    }
}

bool UActorComponent::ResolveLoadedReferences() {
    return true;
}

void UActorComponent::Serialize(FArchive& Archive) {
    UObject::Serialize(Archive);

    Archive.Serialize("bActive", bActive);
}
