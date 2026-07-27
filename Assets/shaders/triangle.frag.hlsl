// Fragment shader: simple PBR-style direct lighting with a pale orange material.

#include "RenderData.hlsli"

struct PSInput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float3 worldPosition : TEXCOORD0;

    [[vk::location(1)]]
    float3 normal        : NORMAL;

    [[vk::location(2)]]
    float3 color         : COLOR0;
};

static const float PI = 3.14159265359f;

float DistributionGGX(float3 normal, float3 halfVector, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(normal, halfVector), 0.0f);
    float nDotH2 = nDotH * nDotH;
    float denom = nDotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * denom * denom, 0.0001f);
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return nDotV / max(nDotV * (1.0f - k) + k, 0.0001f);
}

float GeometrySmith(float3 normal, float3 viewDir, float3 lightDir, float roughness)
{
    float nDotV = max(dot(normal, viewDir), 0.0f);
    float nDotL = max(dot(normal, lightDir), 0.0f);
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (1.0f - f0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float4 main(PSInput input) : SV_Target
{
    const float3 baseColor = float3(0.0f, 0.702f, 0.686f);
    const float metallic = 0.0f;
    const float roughness = 0.48f;
    const float3 cameraPosition =
        cameraData[drawPushConstants.cameraIndex].worldPosition.xyz;
    const float3 lightPosition = float3(2.5f, 3.5f, 2.0f);
    const float3 lightColor = float3(5.0f, 4.6f, 4.2f);
    const float3 ambientColor = float3(0.12f, 0.11f, 0.10f);

    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(cameraPosition - input.worldPosition);
    float3 lightDir = normalize(lightPosition - input.worldPosition);
    float3 halfVector = normalize(viewDir + lightDir);

    float distanceToLight = length(lightPosition - input.worldPosition);
    float attenuation = 1.0f / max(distanceToLight * distanceToLight, 0.0001f);
    float3 radiance = lightColor * attenuation;

    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
    float normalDistribution = DistributionGGX(normal, halfVector, roughness);
    float geometry = GeometrySmith(normal, viewDir, lightDir, roughness);
    float3 fresnel = FresnelSchlick(max(dot(halfVector, viewDir), 0.0f), f0);

    float3 numerator = normalDistribution * geometry * fresnel;
    float denominator = 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f);
    float3 specular = numerator / max(denominator, 0.0001f);

    float3 kS = fresnel;
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    float nDotL = max(dot(normal, lightDir), 0.0f);

    float3 ambient = ambientColor * baseColor;
    float3 color = ambient + (kD * baseColor / PI + specular) * radiance * nDotL;
    color = color / (color + 1.0f);
    color = pow(color, 1.0f / 2.2f);

    return float4(color, 1.0f);
}
