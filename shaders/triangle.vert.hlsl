// Minimal vertex shader: generate a full triangle from SV_VertexID.

struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
    const float2 positions[3] = {
        float2(0.0f, -0.5f),
        float2(0.5f, 0.5f),
        float2(-0.5f, 0.5f)
    };

    VSOutput output;
    output.position = float4(positions[vertexIndex], 0.0f, 1.0f);
    return output;
}
