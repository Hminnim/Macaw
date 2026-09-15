#include "PCH.h"
#include "UCameraComponent.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Subsystem/UCameraSubsystem.h"

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
        Owner->GetWorld()->GetCameraSubsystem().SetMainCamera(this);
    }
}

void UCameraComponent::OnUnregister() {
	UActorComponent::OnUnregister();

    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetCameraSubsystem().ClearMainCamera(this);
    }
}

void UCameraComponent::SetMoveSensitivity(float InMoveSensitivity)
{
    MoveSensitivity = InMoveSensitivity;
}

void UCameraComponent::SetRotationSensitivity(float InRotationSensitivity)
{
    RotationSensitivity = InRotationSensitivity;
}

float UCameraComponent::GetRotationSensitivity() const
{
    return RotationSensitivity;
}

float UCameraComponent::GetMoveSensitivity() const
{
    return MoveSensitivity;
}

void UCameraComponent::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);
    Archive.Serialize("FOV", FOV);
    Archive.Serialize("AspectRatio", AspectRatio);
    Archive.Serialize("NearPlane", NearPlane);
    Archive.Serialize("FarPlane", FarPlane);
}

