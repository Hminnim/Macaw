#pragma once

#include "FMath.h"
#include "Serialize/FArchive.h"

struct FTransform
{
public:
    FTransform() = default;

    FTransform(const FVector3& InPosition, const FRotator& InRotation, const FVector3& InScale)
        : Position(InPosition), Rotation(FQuat::FromRotator(InRotation)), RotationEuler(InRotation), Scale(InScale) {}

    FTransform(const FVector3& InPosition, const FQuat& InRotation, const FVector3& InScale)
        : Position(InPosition), Rotation(InRotation), Scale(InScale) {
        Rotation.Normalize();
        RotationEuler = Rotation.ToRotator();
    }

    const FVector3& GetPosition() const { return Position; }
    const FVector3& GetLocation() const { return Position; }
    const FRotator& GetRotation() const { return RotationEuler; }
    const FQuat& GetRotationQuaternion() const { return Rotation; }
    const FVector3& GetScale() const { return Scale; }
    const FVector3& GetScale3D() const { return Scale; }
    bool IsAbsoluteLocation() const { return bAbsoluteLocation; }
    bool IsAbsoluteRotation() const { return bAbsoluteRotation; }
    bool IsAbsoluteScale() const { return bAbsoluteScale; }

    void SetPosition(const FVector3& InPosition) { Position = InPosition; }
    void SetLocation(const FVector3& Location) { Position = Location; }
    void SetRotation(const FRotator& InRotation);
    void SetRotation(const FQuat& InRotation); 

    void SetScale(const FVector3& InScale) { Scale = InScale; }
    void SetScale3D(const FVector3& Scale) { this->Scale = Scale; }
    void SetAbsoluteLocation(bool bInAbsoluteLocation) { bAbsoluteLocation = bInAbsoluteLocation; }
    void SetAbsoluteRotation(bool bInAbsoluteRotation) { bAbsoluteRotation = bInAbsoluteRotation; }
    void SetAbsoluteScale(bool bInAbsoluteScale) { bAbsoluteScale = bInAbsoluteScale; }

    FMatrix ToMatrixWithScale() const;
    FMatrix ToMatrixNoScale() const;
    FMatrix ToInverseMatrixWithScale() const;
    FTransform Compose(const FTransform& Parent) const;
    bool MakeRelativeTo(const FTransform& Parent, FTransform& OutRelative) const;
    void Serialize(FArchive& Archive);

private:
    FVector3 Position{ 0.0f, 0.0f, 0.0f };
    FQuat Rotation{};
    FRotator RotationEuler{};
    FVector3 Scale{ 1.0f, 1.0f, 1.0f };
    bool bAbsoluteLocation = false;
    bool bAbsoluteRotation = false;
    bool bAbsoluteScale = false;
};
