struct FLight
{
    float3 Color;
    float Intensity;

    float3 Position;
    float AttenuationRadius;

    float3 Direction;
    float InnerConeCos;

    float OuterConeCos;
    uint Type;
    float2 Padding;
};

static const uint LIGHT_DIRECTIONAL = 0;
static const uint LIGHT_POINT = 1;
static const uint LIGHT_SPOT = 2;

StructuredBuffer<FLight> LightContext : register(t2);

float3 CalculateDirectLighting(float3 WorldPosition, float3 Normal, uint LightCount)
{
    float3 NormalizedNormal = normalize(Normal);
    float3 Lighting = 0.0f;

    for (uint LightIndex = 0; LightIndex < LightCount; ++LightIndex)
    {
        FLight Light = LightContext[LightIndex];
        float3 L = 0.0f;
        float Attenuation = 1.0f;

        if (Light.Type == LIGHT_DIRECTIONAL)
        {
            L = normalize(-Light.Direction);
        }
        else
        {
            float3 ToLight = Light.Position - WorldPosition;
            float DistanceToLight = length(ToLight);
            if (Light.AttenuationRadius <= 0.0f || DistanceToLight >= Light.AttenuationRadius)
            {
                continue;
            }

            L = ToLight / max(DistanceToLight, 0.0001f);

            float RadiusFactor = saturate(1.0f - DistanceToLight / Light.AttenuationRadius);
            Attenuation = RadiusFactor * RadiusFactor;

            if (Light.Type == LIGHT_SPOT)
            {
                float SpotFactor = dot(-L, normalize(Light.Direction));
                float Cone = smoothstep(Light.OuterConeCos, Light.InnerConeCos, SpotFactor);
                Attenuation *= Cone;
            }
        }

        Lighting += Light.Color * Light.Intensity * saturate(dot(NormalizedNormal, L)) * Attenuation;
    }

    return Lighting;
}
