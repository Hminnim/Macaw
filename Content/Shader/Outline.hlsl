struct FModelContext
{
    row_major float4x4 World;
    uint MaterialIndex;
    uint Flags;
};

struct FMaterial
{
    float4 BaseColor;
    
    // Paddings
    float4 Parameters0;
    float4 Parameters1;
    float4 Parameters2;
    float4 Parameters3;
    float4 Parameters4;
    float4 Parameters5;
    float4 Parameters6;
};

StructuredBuffer<FModelContext> ModelContexts : register(t0);
StructuredBuffer<FMaterial> MaterialBuffer : register(t1);

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

    const float OutlineWidth = 1.03f;
    
    float3 ExpandedPosition = Input.Position * OutlineWidth;
    
    float4 WorldPosition = mul(float4(ExpandedPosition, 1.0f), ModelContext.World);

    Output.Position = mul(WorldPosition, ViewProjection);
    Output.Normal = mul(Input.Normal, (float3x3) ModelContext.World);
    Output.UV = Input.UV;
    Output.MaterialIndex = ModelContext.MaterialIndex;

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 0.0f, 1.0f);
}