#pragma once

#include "FMath.h"
#include "Serialize/FArchive.h"

struct FTransform
{
public:
    FTransform() = default;

    FTransform(const FVector3& InPosition, const FRotator& InRotation, const FVector3& InScale) : Position(InPosition), Rotation(InRotation), Scale(InScale) {}

    const FVector3& GetPosition() const { return Position; }
    const FVector3& GetLocation() const { return Position; }
    const FRotator& GetRotation() const { return Rotation; }
    const FVector3& GetScale() const { return Scale; }
    const FVector3& GetScale3D() const { return Scale; }

    void SetPosition(const FVector3& InPosition) { Position = InPosition; }
    void SetLocation(const FVector3& Location) { Position = Location; }
    void SetRotation(const FRotator& InRotation) { Rotation = InRotation; }
    void SetScale(const FVector3& InScale) { Scale = InScale; }
    void SetScale3D(const FVector3& Scale) { this->Scale = Scale; }

    FMatrix ToMatrixWithScale() const;
    FMatrix ToMatrixNoScale() const;
    FMatrix ToInverseMatrixWithScale() const;
    void Serialize(FArchive& Archive);

private:
    FVector3 Position{ 0.0f, 0.0f, 0.0f };
    FRotator Rotation{ 0.0f, 0.0f, 0.0f };
    FVector3 Scale{ 1.0f, 1.0f, 1.0f };
};
