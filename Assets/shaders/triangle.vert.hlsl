// Vertex shader: selects per-camera and per-object data through draw indices.

#include "RenderData.hlsli"

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION;

    [[vk::location(1)]]
    float4 color    : COLOR0;

    [[vk::location(2)]]
    float3 normal   : NORMAL;

    [[vk::location(3)]]
    float2 texCoord : TEXCOORD0;

    [[vk::location(4)]]
    float4 tangent  : TANGENT;

};

struct VSOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float3 worldPosition : TEXCOORD0;

    [[vk::location(1)]]
    float3 normal        : NORMAL;

    [[vk::location(2)]]
    float4 color         : COLOR0;

    [[vk::location(3)]]
    float2 texCoord      : TEXCOORD1;

    [[vk::location(4)]]
    float4 tangent       : TANGENT;
};

VSOutput main(VSInput input)
{
    const CameraGpuData camera =
        cameraBuffer.cameras[drawPushConstants.cameraIndex];
    const ObjectGpuData object = objectData[drawPushConstants.objectIndex];

    VSOutput output;
    float4 worldPosition = mul(object.world, float4(input.position, 1.0));
    output.position = mul(camera.viewProjection, worldPosition);
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul(
        (float3x3)object.normalMatrix,
        input.normal));
    float3 worldTangent = mul(
        (float3x3)object.world,
        input.tangent.xyz);
    worldTangent = normalize(
        worldTangent - output.normal * dot(output.normal, worldTangent));
    const float transformHandedness =
        determinant((float3x3)object.world) < 0.0f ? -1.0f : 1.0f;
    output.tangent = float4(
        worldTangent,
        input.tangent.w * transformHandedness);
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}
