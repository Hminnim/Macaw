struct FBillboardData
{
    row_major float4x4 World;
    float2 Size;
    float2 UVMin;
    float2 UVMax;
    float2 Pad;
    float4 Color;
};

StructuredBuffer<FBillboardData> Billboards : register(t0);

Texture2D SpriteTexture : register(t2);
SamplerState LinearClamp : register(s1);

cbuffer BillboardViewConstans : register(b0)
{
    row_major float4x4 ViewProjection;
    row_major float4x4 CameraWorld;
};

struct VS_OUTPUT
{
    uint InstanceID : INSTANCE_ID;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
};

VS_OUTPUT mainVS(uint InstanceID : SV_InstanceID)
{
    VS_OUTPUT Output;    
    Output.InstanceID = InstanceID;    
    return Output;
}

[maxvertexcount(4)] 
void mainGS(point VS_OUTPUT Input[1], inout TriangleStream<PS_INPUT> Stream)
{
    const uint BillboardIndex = Input[0].InstanceID;
    const FBillboardData Data = Billboards[BillboardIndex];

}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float4 AtlasColor = FontAtlas.SampleLevel(PointClamp, Input.UV, 0.0f); // Atlas 텍스처의 Input.UV 위치 색상을 읽어라.
    float Coverage = AtlasColor.r;
    
    return float4(TextColor.rgb, TextColor.a * Coverage);
}
