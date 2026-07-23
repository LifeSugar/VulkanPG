// Vertex shader: selects per-camera and per-object data through draw indices.

#include "RenderData.hlsli"

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION;

    [[vk::location(1)]]
    float3 color    : COLOR0;

    [[vk::location(2)]]
    float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float3 worldPosition : TEXCOORD0;

    [[vk::location(1)]]
    float3 normal        : NORMAL;

    [[vk::location(2)]]
    float3 color         : COLOR0;
};

VSOutput main(VSInput input)
{
    const CameraGpuData camera = cameraData[drawPushConstants.cameraIndex];
    const ObjectGpuData object = objectData[drawPushConstants.objectIndex];

    VSOutput output;
    float4 worldPosition = mul(object.world, float4(input.position, 1.0));
    output.position = mul(camera.viewProjection, worldPosition);
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul((float3x3)object.normalMatrix, input.normal));
    output.color = input.color;
    return output;
}
