#pragma once

#include "FArchive.h"
#include "../Common.h"
#include "../FMath.h"
struct FEditorSettings {
	float32 MoveSensitivity = 5.0f;
	float32 RotationSensitivity = 1.0f;
	FVector CameraStartPosition = FVector(0.0f, 0.0f, 0.0f);
	float32 GridSize = 1.f;
	FString LastLoadedScenePath = "";

	void Serialize(FArchive& Ar) {
		Ar.Serialize("MoveSensitivity", MoveSensitivity);
		Ar.Serialize("RotationSensitivity", RotationSensitivity);
		Ar.Serialize("CameraStartPosition", CameraStartPosition);
		Ar.Serialize("GridSize", GridSize);
		Ar.Serialize("LastLoadedScenePath", LastLoadedScenePath);
	}
};