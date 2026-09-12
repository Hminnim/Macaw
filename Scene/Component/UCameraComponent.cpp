#include "PCH.h"
#include "UCameraComponent.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"

UCameraComponent::UCameraComponent() {
    SetRelativeLocation({ 5.0f, 5.0f, 5.0f });
}

FMatrix UCameraComponent::GetViewMatrix() const {
    return GetComponentToWorld().Invert();
}

FMatrix UCameraComponent::GetProjectionMatrix() const {
    return FMatrix::CreatePerspectiveFieldOfView(
        FOV,
        AspectRatio,
        NearPlane,
        FarPlane
    );
}

FMatrix UCameraComponent::GetViewProjectionMatrix() const {
    return GetViewMatrix() * GetProjectionMatrix();
}

float UCameraComponent::GetFOV() const {
    return FOV;
}

float UCameraComponent::GetAspectRatio() const {
    return AspectRatio;
}

float UCameraComponent::GetNearPlane() const {
    return NearPlane;
}

float UCameraComponent::GetFarPlane() const {
    return FarPlane;
}

void UCameraComponent::SetFOV(float InFOV) {
    FOV = InFOV;
}

void UCameraComponent::SetAspectRatio(float InAspectRatio) {
    AspectRatio = InAspectRatio;
}

void UCameraComponent::SetNearPlane(float InNearPlane) {
    NearPlane = InNearPlane;
}

void UCameraComponent::SetFarPlane(float InFarPlane) {
    FarPlane = InFarPlane;
}

void UCameraComponent::OnRegister() {
    UActorComponent::OnRegister();
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->SetMainCamera(this);
    }
}

void UCameraComponent::OnUnregister() {
	UActorComponent::OnUnregister();

    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->ClearMainCamera(this);
    }
}

void UCameraComponent::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);
    Archive.Serialize("FOV", FOV);
    Archive.Serialize("AspectRatio", AspectRatio);
    Archive.Serialize("NearPlane", NearPlane);
    Archive.Serialize("FarPlane", FarPlane);
}

