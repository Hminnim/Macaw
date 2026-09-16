#pragma once 
#include "../Asset/FAssetHandle.h"

struct FTextVertex
{
    // 텍스트 원점으로부터 글자의 상대 위치
    FVector2 LocalPosition{};

    // 글자 Quad의 월드 크기
    FVector2 Size{};

    // Atlas의 문자 UV 범위
    FVector2 UVMin{};
    FVector2 UVMax{};
};

struct FTextProbe
{
    // UBillBoardTextComponent의 렌더링 원점으로 사용할 World Transform
    FMatrix World{};

    // 사용할 UFont
    FAssetHandle FontHandle{};

    // Text Geometry Shader Pipeline
    FAssetHandle PipelineHandle{};
    FVector4 Color{ 1.0f,1.0f,1.0f,1.0f };
    TArray<FTextVertex> Vertices{};
};

struct FBillboardProbe
{
    FMatrix World{};

    FAssetHandle TextureHandle{};
    FAssetHandle PipelineHandle{};

    FVector2 Size;
    FVector2 UVMin;
    FVector2 UVMax;
    FVector4 Color;
};

enum class ERenderObjectFlags : uint32 {
	None = 0,
	Selected = 1u << 0
};

enum class ERenderLayer : uint32 {
	None = 0,
    Sky = 1,
	Opaque = 2,
	Transparent = 3,
	Gizmo = 4
};

// GPU light records use this value directly, so keep the underlying type
// compatible with HLSL's uint.
enum class ELightType : uint32 {
    Directional,
    Point,
    Spot
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

// World-space, renderer-facing light data. This layout deliberately matches
// the StructuredBuffer element consumed by the lighting shader.
struct FLightProbe {
    FVector3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity{ 1.0f };

    FVector3 Position{};
    float AttenuationRadius{};

    FVector3 Direction{ 0.0f, 0.0f, 1.0f };
    float InnerConeCos{ 1.0f };

    float OuterConeCos{ 1.0f };
    ELightType Type{ ELightType::Directional };
    FVector2 Padding{};
};

static_assert(sizeof(FLightProbe) == 64);

struct FRenderProbe {
	TArray<FActorProbe> ActorProbes{};
	TArray<FActorProbe> GizmoProbes{};
    TArray<FTextProbe> TextProbes{};
    TArray<FBillboardProbe> BillboardProbes{};
    TArray<FLightProbe> LightProbes{};

	CameraProbe MainCameraProbe{}; 
};
