#include "PCH.h"

#include "UWorldSubsystem.h"

void UWorldSubsystem::Initialize(UWorld* World) {
    if (bInitialized || World == nullptr) {
        return;
    }

    this->World = World;
    bInitialized = true;
    OnInitialize();
}

void UWorldSubsystem::Deinitialize() {
    if (!bInitialized) {
        return;
    }

    OnDeinitialize();
    bInitialized = false;
    World = nullptr;
}

UWorld* UWorldSubsystem::GetWorld() const {
    return World;
}

bool UWorldSubsystem::IsInitialized() const {
    return bInitialized;
}

void UWorldSubsystem::OnInitialize() {
}

void UWorldSubsystem::OnDeinitialize() {
}
