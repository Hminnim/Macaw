cbuffer LineFrameConstants : register(b0)
{
    row_major float4x4 ViewProjection;
    float4 Viewport;
};

struct VS_INPUT
{
    float2 Corner : CORNER;
    float4 StartAndWidth : START;
    float4 EndAndPadding : END;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    noperspective float EdgeDistance : TEXCOORD0;
    nointerpolation float HalfWidth : TEXCOORD1;
};

bool ClipLineToNearPlane(inout float4 StartClip, inout float4 EndClip)
{
    const float NearClipEpsilon = 0.00001f;

    float StartDistance = StartClip.z - NearClipEpsilon;
    float EndDistance = EndClip.z - NearClipEpsilon;

    bool StartBehindNearPlane = StartDistance < 0.0f;
    bool EndBehindNearPlane = EndDistance < 0.0f;

    if (StartBehindNearPlane && EndBehindNearPlane)
    {
        return false;
    }

    if (StartBehindNearPlane)
    {
        float Interpolation = saturate(-StartDistance / (EndDistance - StartDistance));
        StartClip = lerp(StartClip, EndClip, Interpolation);
    }
    else if (EndBehindNearPlane)
    {
        float Interpolation = saturate(-EndDistance / (StartDistance - EndDistance));
        EndClip = lerp(EndClip, StartClip, Interpolation);
    }

    return true;
}

PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    float4 StartClip = mul(float4(Input.StartAndWidth.xyz, 1.0f), ViewProjection);
    float4 EndClip = mul(float4(Input.EndAndPadding.xyz, 1.0f), ViewProjection);

    if (!ClipLineToNearPlane(StartClip, EndClip))
    {
        Output.Position = float4(0.0f, 0.0f, -1.0f, 1.0f);
        Output.Color = float4(Input.Color.rgb, 0.0f);
        Output.EdgeDistance = 0.0f;
        Output.HalfWidth = 0.0f;
        return Output;
    }

    float2 StartNDC = StartClip.xy / StartClip.w;
    float2 EndNDC = EndClip.xy / EndClip.w;

    float2 ScreenDirection = (EndNDC - StartNDC) * float2(Viewport.x, -Viewport.y);
    float ScreenLength = length(ScreenDirection);

    ScreenDirection = ScreenLength > 0.0001f ? ScreenDirection / ScreenLength : float2(1.0f, 0.0f);

    float2 ScreenNormal = float2(-ScreenDirection.y, ScreenDirection.x);

    float HalfWidth = max(Input.StartAndWidth.w * 0.5f, 0.5f);
    float RasterHalfWidth = HalfWidth + 1.0f;

    float2 OffsetPixels = ScreenNormal * Input.Corner.y * RasterHalfWidth;
    float2 OffsetNDC = OffsetPixels * float2(2.0f * Viewport.z, -2.0f * Viewport.w);

    float4 BaseClip = Input.Corner.x < 0.5f ? StartClip : EndClip;
    BaseClip.xy += OffsetNDC * BaseClip.w;

    Output.Position = BaseClip;
    Output.Color = Input.Color;
    Output.EdgeDistance = Input.Corner.y * RasterHalfWidth;
    Output.HalfWidth = HalfWidth;

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float Coverage = 1.0f - smoothstep(
        Input.HalfWidth - 0.5f,
        Input.HalfWidth + 0.5f,
        abs(Input.EdgeDistance));

    float4 FinalColor = Input.Color;
    FinalColor.a *= Coverage;

    return FinalColor;
}
