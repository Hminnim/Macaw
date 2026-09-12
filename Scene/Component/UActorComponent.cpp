#include "PCH.h"
#include "UActorComponent.h"
#include "../AActor.h"
#include "../../ErrorHandler.h"

AActor* UActorComponent::GetOwner() const {
    return Owner;
}

void UActorComponent::SetOwner(AActor* InOwner) {
    Owner = InOwner;
}

void UActorComponent::OnRegister() {
}

void UActorComponent::Tick(float /*DeltaTime*/) {
}

void UActorComponent::OnUnregister() {
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
    ErrorHandler::Report(Owner == nullptr or ParentWorld == nullptr, "[ UActorComponent ]", "Owner and ParentWorld must not be null.", ErrorHandler::EErrorLevel::Critical);

    if (not bRegistered) return;

    this->OnUnregister();
    bRegistered = false;
    ParentWorld = nullptr;
}

bool UActorComponent::ResolveLoadedReferences() {
    return true;
}

void UActorComponent::Serialize(FArchive& Archive) {
    UObject::Serialize(Archive);

    Archive.Serialize("bActive", bActive);
}
