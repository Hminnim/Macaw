#include "PCH.h"
#include "FMath.h"
#include "FTransform.h"
#include "Serialize/FArchive.h"

namespace {
    FMatrix MakeTransformMatrix(const FVector3& Position, const FRotator& Rotation, const FVector3& Scale) {
        FMatrix ScaleMatrix = FMatrix::CreateScale(Scale);

        FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Rotation.y, Rotation.x,Rotation.z);

        FMatrix TranslationMatrix = FMatrix::CreateTranslation(Position);
        FMatrix SourceYUpToWorldZUp = FMatrix::CreateYUpToZUp();
        return ScaleMatrix * RotationMatrix * SourceYUpToWorldZUp * TranslationMatrix;
    }
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

void FTransform::Serialize(FArchive& Archive) {
    Archive.Serialize("Position", Position);
    Archive.Serialize("Rotation", Rotation);
    Archive.Serialize("Scale", Scale);
}
