#include "PCH.h"
#include "FMath.h"
#include "FTransform.h"
#include "Serialize/FArchive.h"

FMatrix FTransform::GetWorldMatrix() const
{
	FMatrix S = FMatrix::CreateScale(Scale);

    FMatrix R = FMatrix::CreateFromYawPitchRoll(
        Rotation.y,
        Rotation.x,
        Rotation.z);

    FMatrix T = FMatrix::CreateTranslation(Position);

    FMatrix U = FMatrix::CreateYUpToZUp();
    return S * R * U * T;
}

void FTransform::Serialize(FArchive& Archive)
{
    Archive.Serialize("Position", Position);
    Archive.Serialize("Rotation", Rotation);
    Archive.Serialize("Scale", Scale);
}
