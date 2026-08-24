#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute
};

enum class ShaderValueType
{
    Unknown,
    Float,
    Float2,
    Float3,
    Float4,
    Matrix4,
    Int,
    UInt,
    Bool
};

enum class ShaderResourceType
{
    UniformBuffer,
    StorageBuffer,
    SampledImage,
    Sampler,
    CombinedImageSampler
};

struct ShaderResourceDesc
{
    std::string name;
    uint32_t set = 0;
    uint32_t binding = 0;
    ShaderResourceType type = ShaderResourceType::UniformBuffer;
    uint32_t arrayCount = 1;
    ShaderStage stage = ShaderStage::Vertex;
};

struct ShaderBlockMemberDesc
{
    std::string name;
    ShaderValueType type = ShaderValueType::Unknown;
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct ShaderParameterBlockDesc
{
    std::string name;
    uint32_t set = 0;
    uint32_t binding = 0;
    uint32_t byteSize = 0;
    ShaderStage stage = ShaderStage::Vertex;
    std::vector<ShaderBlockMemberDesc> members;
};

struct ShaderStageIoDesc
{
    std::string name;
    uint32_t location = 0;
    ShaderValueType type = ShaderValueType::Unknown;
};

struct ShaderPushConstantDesc
{
    std::string name;
    uint32_t byteSize = 0;
    ShaderStage stage = ShaderStage::Vertex;
};

/// Backend-independent interface reflected from one SPIR-V entry point.
struct ShaderInterface
{
    ShaderStage stage = ShaderStage::Vertex;
    std::string entryPoint = "main";
    std::vector<ShaderResourceDesc> resources;
    std::vector<ShaderParameterBlockDesc> parameterBlocks;
    std::vector<ShaderStageIoDesc> inputs;
    std::vector<ShaderStageIoDesc> outputs;
    std::vector<ShaderPushConstantDesc> pushConstants;
    uint64_t signature = 0;
};

} // namespace VkRenderer
