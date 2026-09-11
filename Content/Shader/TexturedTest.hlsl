struct FModelContext
{
    row_major float4x4 World;
    uint MaterialIndex;
    uint Flags;
};

StructuredBuffer<FModelContext> ModelContexts : register(t0);
Texture2D BaseColorTexture : register(t2);
SamplerState LinearWrap : register(s0);

cbuffer RootConstants : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    row_major float4x4 ViewProjection;

    uint ModelContextStart;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    nointerpolation uint MaterialIndex : Jungle1;
    nointerpolation float3 ColorCoefficient : Jungle2;
};

PS_INPUT mainVS(VS_INPUT Input, uint InstanceID : SV_InstanceID)
{
    PS_INPUT Output;

    FModelContext ModelContext = ModelContexts[ModelContextStart + InstanceID];
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), ModelContext.World);

    Output.Position = mul(WorldPosition, ViewProjection);
    Output.Normal = mul(Input.Normal, (float3x3)ModelContext.World);
    Output.UV = Input.UV;
    Output.MaterialIndex = ModelContext.MaterialIndex;
    Output.ColorCoefficient = (ModelContext.Flags & 1) != 0
        ? float3(1.0f, 0.0f, 0.0f)
        : float3(1.0f, 1.0f, 1.0f);

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float4 Color = BaseColorTexture.Sample(LinearWrap, Input.UV);
    Color.rgb *= Input.ColorCoefficient;
    return Color;
}
