#include "PCH.h"
#include "FMath.h"
#include "FTransform.h"
#include "Serialize/FArchive.h"

namespace {
    FMatrix MakeTransformMatrix(const FVector3& Position, const FQuat& Rotation, const FVector3& Scale) {
        FMatrix ScaleMatrix = FMatrix::CreateScale(Scale);

        FMatrix RotationMatrix = FMatrix::CreateFromQuaternion(Rotation);

        FMatrix TranslationMatrix = FMatrix::CreateTranslation(Position);
        FMatrix SourceYUpToWorldZUp = FMatrix::CreateYUpToZUp();
        return ScaleMatrix * RotationMatrix * SourceYUpToWorldZUp * TranslationMatrix;
    }
}

void FTransform::SetRotation(const FRotator& InRotation) {
    RotationEuler = InRotation;
    Rotation = FQuat::FromRotator(InRotation);
}

void FTransform::SetRotation(const FQuat& InRotation) {
    Rotation = InRotation;
    Rotation.Normalize();
    RotationEuler = Rotation.ToRotator();
}

FMatrix FTransform::ToMatrixWithScale() const {
    return MakeTransformMatrix(Position, Rotation, Scale);
}

FMatrix FTransform::ToMatrixNoScale() const {
    return MakeTransformMatrix(Position, Rotation, { 1.0f, 1.0f, 1.0f });
}

FMatrix FTransform::ToInverseMatrixWithScale() const {
    return ToMatrixWithScale().Invert();
}

FTransform FTransform::Compose(const FTransform& Parent) const {
    const FVector3 ScaledPosition{
        Position.x * Parent.Scale.x,
        Position.y * Parent.Scale.y,
        Position.z * Parent.Scale.z
    };
    
    const FVector3 WorldPosition = bAbsoluteLocation
        ? Position
        : FMatrix::CreateFromQuaternion(Parent.Rotation).TransformDirection(ScaledPosition) + Parent.Position;

    const FVector3 WorldScale{
        Scale.x * Parent.Scale.x,
        Scale.y * Parent.Scale.y,
        Scale.z * Parent.Scale.z
    };

    FTransform WorldTransform{
        WorldPosition,
        bAbsoluteRotation ? Rotation : FQuat::Concatenate(Rotation, Parent.Rotation),
        bAbsoluteScale ? Scale : WorldScale
    };
    WorldTransform.SetAbsoluteLocation(bAbsoluteLocation);
    WorldTransform.SetAbsoluteRotation(bAbsoluteRotation);
    WorldTransform.SetAbsoluteScale(bAbsoluteScale);
    return WorldTransform;
}

bool FTransform::MakeRelativeTo(const FTransform& Parent, FTransform& OutRelative) const {
    constexpr float Epsilon = 1e-6f;
    if ((!bAbsoluteLocation || !bAbsoluteScale) &&
        (std::abs(Parent.Scale.x) <= Epsilon || std::abs(Parent.Scale.y) <= Epsilon || std::abs(Parent.Scale.z) <= Epsilon)) {
        return false;
    }

    FVector3 RelativePosition = Position;
    if (!bAbsoluteLocation) {
        const FVector3 ParentSpacePosition = FMatrix::CreateFromQuaternion(Parent.Rotation.Inverse()).TransformDirection(Position - Parent.Position);
        RelativePosition = {
            ParentSpacePosition.x / Parent.Scale.x,
            ParentSpacePosition.y / Parent.Scale.y,
            ParentSpacePosition.z / Parent.Scale.z
        };
    }

    FVector3 RelativeScale = Scale;
    if (!bAbsoluteScale) {
        RelativeScale = {
            Scale.x / Parent.Scale.x,
            Scale.y / Parent.Scale.y,
            Scale.z / Parent.Scale.z
        };
    }

    OutRelative = {
        RelativePosition,
        bAbsoluteRotation ? Rotation : FQuat::Concatenate(Rotation, Parent.Rotation.Inverse()),
        RelativeScale
    };
    OutRelative.SetAbsoluteLocation(bAbsoluteLocation);
    OutRelative.SetAbsoluteRotation(bAbsoluteRotation);
    OutRelative.SetAbsoluteScale(bAbsoluteScale);
    return true;
}

void FTransform::Serialize(FArchive& Archive) {
    Archive.Serialize("Position", Position);
    FVector3 SerializedRotation = RotationEuler;
    Archive.Serialize("Rotation", SerializedRotation);
    Archive.Serialize("Scale", Scale);
    Archive.Serialize("bAbsoluteLocation", bAbsoluteLocation);
    Archive.Serialize("bAbsoluteRotation", bAbsoluteRotation);
    Archive.Serialize("bAbsoluteScale", bAbsoluteScale);

    if (Archive.IsLoading()) {
        SetRotation(FRotator(SerializedRotation));
    }
}
