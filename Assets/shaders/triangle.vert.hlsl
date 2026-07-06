// Vertex shader: receives mesh data from the vertex buffer and MVP from set 0 binding 0.

#pragma pack_matrix(column_major)

[[vk::binding(0, 0)]]
cbuffer MVPUniformBuffer : register(b0, space0)
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

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
    VSOutput output;
    float4 worldPosition = mul(model, float4(input.position, 1.0));
    float4 viewPosition = mul(view, worldPosition);
    output.position = mul(proj, viewPosition);
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul((float3x3)model, input.normal));
    output.color = input.color;
    return output;
}
