#include "PCH.h"

#include "UCameraSubsystem.h"

void UCameraSubsystem::SetMainCamera(UCameraComponent* Camera) {
    MainCamera = Camera;
}

void UCameraSubsystem::ClearMainCamera(UCameraComponent* Camera) {
    if (MainCamera == Camera) {
        MainCamera = nullptr;
    }
}

UCameraComponent* UCameraSubsystem::GetMainCamera() const {
    return MainCamera;
}

void UCameraSubsystem::OnDeinitialize() {
    MainCamera = nullptr;
}
