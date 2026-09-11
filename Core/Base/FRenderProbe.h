#pragma once 
#include "../Asset/FAssetHandle.h"

struct FActorProbe {
	FMatrix World;
	FAssetHandle MeshHandle;
	FAssetHandle MaterialHandle;
	FAssetHandle PipelineHandle;
	uint32 Flags{ 0x0000'0000 };
};

struct CameraProbe {
	FMatrix ViewProjection{}; 
	FMatrix View{};
	FMatrix Projection{};
};

struct FRenderProbe {
	TArray<FActorProbe> ActorProbes{};
	TArray<FActorProbe> GizmoProbes{};
	CameraProbe MainCameraProbe{}; 
};