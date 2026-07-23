#ifndef LEARN_VULKAN_RENDER_DATA_HLSLI
#define LEARN_VULKAN_RENDER_DATA_HLSLI

#pragma pack_matrix(column_major)

struct CameraGpuData
{
    float4x4 viewProjection;
    float4 worldPosition;
};

struct ObjectGpuData
{
    float4x4 world;
    float4x4 normalMatrix;
};

struct DrawPushConstants
{
    uint cameraIndex;
    uint objectIndex;
};

// set 0 / binding 0 is reserved for future per-frame constants.
[[vk::binding(1, 0)]]
StructuredBuffer<CameraGpuData> cameraData : register(t1, space0);

[[vk::binding(2, 0)]]
StructuredBuffer<ObjectGpuData> objectData : register(t2, space0);

[[vk::push_constant]]
ConstantBuffer<DrawPushConstants> drawPushConstants;

#endif
