// Final SDR presentation shader.
//
// Color contract:
//   1. sceneColorTexture contains linear scene-referred RGB values. A typical
//      backing format is VK_FORMAT_R16G16B16A16_SFLOAT.
//   2. Exposure and tone mapping produce linear display-referred RGB values.
//   3. For an *_SRGB swapchain format, set outputTransferFunction to 0. The
//      color attachment performs the linear-to-sRGB conversion on write.
//   4. For an *_UNORM swapchain paired with SRGB_NONLINEAR color space, set
//      outputTransferFunction to 1 so this shader performs that conversion.
//
// Never enable both shader sRGB encoding and an *_SRGB attachment: that would
// apply the transfer function twice and make the image too bright/washed out.

struct PresentPixelInput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float2 texCoord : TEXCOORD0;
};

struct PresentPushConstants
{
    // Exposure in stops. 1.0 doubles scene luminance, -1.0 halves it.
    float exposureEv;

    // 0: no tone mapping (clamp only), 1: ACES fitted approximation.
    uint toneMappingMode;

    // 0: output linear RGB; *_SRGB attachment encodes it.
    // 1: explicitly encode linear RGB to sRGB for an *_UNORM attachment.
    uint outputTransferFunction;

    uint padding;
};

[[vk::binding(0, 0)]]
Texture2D<float4> sceneColorTexture : register(t0, space0);

[[vk::binding(1, 0)]]
SamplerState sceneColorSampler : register(s1, space0);

[[vk::push_constant]]
ConstantBuffer<PresentPushConstants> presentPushConstants;

float3 ToneMapAcesFitted(float3 color)
{
    // Narkowicz ACES approximation. The result is display-linear and remains
    // subject to the selected output transfer function below.
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((color * (a * color + b)) /
                    (color * (c * color + d) + e));
}

float3 LinearToSrgb(float3 linearColor)
{
    // IEC 61966-2-1 transfer function. Evaluate the power branch with a
    // non-negative input even though select/lerp implementations may execute
    // both branches.
    const float3 safeColor = max(linearColor, 0.0f);
    const float3 low = safeColor * 12.92f;
    const float3 high =
        1.055f * pow(safeColor, 1.0f / 2.4f) - 0.055f;
    return float3(
        safeColor.r <= 0.0031308f ? low.r : high.r,
        safeColor.g <= 0.0031308f ? low.g : high.g,
        safeColor.b <= 0.0031308f ? low.b : high.b);
}

float4 main(PresentPixelInput input) : SV_Target
{
    const float4 sceneSample =
        sceneColorTexture.Sample(sceneColorSampler, input.texCoord);

    float3 displayLinear =
        max(sceneSample.rgb, 0.0f) * exp2(presentPushConstants.exposureEv);

    if (presentPushConstants.toneMappingMode == 1)
    {
        displayLinear = ToneMapAcesFitted(displayLinear);
    }
    else
    {
        displayLinear = saturate(displayLinear);
    }

    float3 outputColor = displayLinear;
    if (presentPushConstants.outputTransferFunction == 1)
    {
        outputColor = LinearToSrgb(displayLinear);
    }

    // The current swapchain uses opaque composite alpha. Keeping alpha at one
    // also avoids accidentally coupling window composition to scene alpha.
    return float4(outputColor, 1.0f);
}
