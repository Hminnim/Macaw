#pragma once 
#include "../Asset/FAssetHandle.h"

enum class ERenderObjectFlags : uint32 {
	None = 0,
	Selected = 1u << 0
};

constexpr uint32 operator|(ERenderObjectFlags Left, ERenderObjectFlags Right) {
	return static_cast<uint32>(Left) | static_cast<uint32>(Right);
}

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
