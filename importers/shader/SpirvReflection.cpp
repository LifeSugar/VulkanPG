#include "shader/SpirvReflection.hpp"

#include <spirv_cross.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace VkRenderer
{
namespace
{

[[nodiscard]] spv::ExecutionModel executionModel(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex: return spv::ExecutionModelVertex;
    case ShaderStage::Fragment: return spv::ExecutionModelFragment;
    case ShaderStage::Compute: return spv::ExecutionModelGLCompute;
    }
    throw std::invalid_argument("unsupported shader stage");
}

[[nodiscard]] ShaderValueType valueType(
    const spirv_cross::SPIRType& type) noexcept
{
    using BaseType = spirv_cross::SPIRType::BaseType;
    if (type.columns == 4 && type.vecsize == 4 &&
        type.basetype == BaseType::Float)
    {
        return ShaderValueType::Matrix4;
    }
    if (type.columns != 1)
    {
        return ShaderValueType::Unknown;
    }

    if (type.basetype == BaseType::Float)
    {
        switch (type.vecsize)
        {
        case 1: return ShaderValueType::Float;
        case 2: return ShaderValueType::Float2;
        case 3: return ShaderValueType::Float3;
        case 4: return ShaderValueType::Float4;
        default: return ShaderValueType::Unknown;
        }
    }
    if (type.vecsize != 1)
    {
        return ShaderValueType::Unknown;
    }
    switch (type.basetype)
    {
    case BaseType::Int: return ShaderValueType::Int;
    case BaseType::UInt: return ShaderValueType::UInt;
    case BaseType::Boolean: return ShaderValueType::Bool;
    default: return ShaderValueType::Unknown;
    }
}

[[nodiscard]] uint32_t arrayCount(
    const spirv_cross::SPIRType& type) noexcept
{
    if (type.array.empty())
    {
        return 1;
    }

    uint64_t count = 1;
    for (std::size_t index = 0; index < type.array.size(); ++index)
    {
        if (index >= type.array_size_literal.size() ||
            !type.array_size_literal[index] || type.array[index] == 0)
        {
            return 0;
        }
        count *= type.array[index];
        if (count > std::numeric_limits<uint32_t>::max())
        {
            return 0;
        }
    }
    return static_cast<uint32_t>(count);
}

[[nodiscard]] std::string resourceName(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::Resource& resource)
{
    if (!resource.name.empty())
    {
        return resource.name;
    }
    return compiler.get_name(resource.id);
}

void appendResource(
    ShaderInterface& result,
    const spirv_cross::Compiler& compiler,
    const spirv_cross::Resource& resource,
    ShaderResourceType resourceType)
{
    const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
    result.resources.push_back({
        resourceName(compiler, resource),
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet),
        compiler.get_decoration(resource.id, spv::DecorationBinding),
        resourceType,
        arrayCount(type),
        result.stage
    });
}

void appendParameterBlock(
    ShaderInterface& result,
    const spirv_cross::Compiler& compiler,
    const spirv_cross::Resource& resource)
{
    const spirv_cross::SPIRType& blockType =
        compiler.get_type(resource.base_type_id);
    ShaderParameterBlockDesc block{};
    block.name = resourceName(compiler, resource);
    block.set =
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    block.binding =
        compiler.get_decoration(resource.id, spv::DecorationBinding);
    block.byteSize = static_cast<uint32_t>(
        compiler.get_declared_struct_size(blockType));
    block.stage = result.stage;
    block.members.reserve(blockType.member_types.size());
    for (uint32_t memberIndex = 0;
         memberIndex < static_cast<uint32_t>(blockType.member_types.size());
         ++memberIndex)
    {
        const spirv_cross::SPIRType& memberType =
            compiler.get_type(blockType.member_types[memberIndex]);
        block.members.push_back({
            compiler.get_member_name(resource.base_type_id, memberIndex),
            valueType(memberType),
            compiler.type_struct_member_offset(blockType, memberIndex),
            static_cast<uint32_t>(
                compiler.get_declared_struct_member_size(
                    blockType,
                    memberIndex))
        });
    }
    result.parameterBlocks.push_back(std::move(block));
}

void appendStageIo(
    std::vector<ShaderStageIoDesc>& destination,
    const spirv_cross::Compiler& compiler,
    const spirv_cross::Resource& resource)
{
    if (!compiler.has_decoration(resource.id, spv::DecorationLocation))
    {
        return;
    }
    destination.push_back({
        resourceName(compiler, resource),
        compiler.get_decoration(resource.id, spv::DecorationLocation),
        valueType(compiler.get_type(resource.type_id))
    });
}

void hashBytes(uint64_t& hash, const void* data, std::size_t size) noexcept
{
    constexpr uint64_t kFnvPrime = UINT64_C(1099511628211);
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t index = 0; index < size; ++index)
    {
        hash ^= bytes[index];
        hash *= kFnvPrime;
    }
}

template <typename Value>
void hashValue(uint64_t& hash, const Value& value) noexcept
{
    hashBytes(hash, &value, sizeof(value));
}

void hashString(uint64_t& hash, const std::string& value) noexcept
{
    hashBytes(hash, value.data(), value.size());
    const unsigned char terminator = 0;
    hashBytes(hash, &terminator, 1);
}

[[nodiscard]] uint64_t interfaceSignature(const ShaderInterface& interface)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    hashValue(hash, interface.stage);
    hashString(hash, interface.entryPoint);

    std::vector<ShaderResourceDesc> resources = interface.resources;
    std::sort(
        resources.begin(),
        resources.end(),
        [](const auto& left, const auto& right)
        {
            return std::tie(
                left.set,
                left.binding,
                left.type,
                left.name) <
                std::tie(
                    right.set,
                    right.binding,
                    right.type,
                    right.name);
        });
    for (const ShaderResourceDesc& resource : resources)
    {
        hashString(hash, resource.name);
        hashValue(hash, resource.set);
        hashValue(hash, resource.binding);
        hashValue(hash, resource.type);
        hashValue(hash, resource.arrayCount);
    }

    std::vector<ShaderParameterBlockDesc> blocks = interface.parameterBlocks;
    std::sort(
        blocks.begin(),
        blocks.end(),
        [](const auto& left, const auto& right)
        {
            return std::tie(left.set, left.binding, left.name) <
                std::tie(right.set, right.binding, right.name);
        });
    for (const ShaderParameterBlockDesc& block : blocks)
    {
        hashString(hash, block.name);
        hashValue(hash, block.set);
        hashValue(hash, block.binding);
        hashValue(hash, block.byteSize);
        for (const ShaderBlockMemberDesc& member : block.members)
        {
            hashString(hash, member.name);
            hashValue(hash, member.type);
            hashValue(hash, member.offset);
            hashValue(hash, member.size);
        }
    }
    return hash;
}

} // namespace

ShaderInterface SpirvReflection::reflect(
    const std::vector<uint32_t>& spirv,
    ShaderStage stage,
    const std::string& entryPoint)
{
    if (spirv.empty() || entryPoint.empty())
    {
        throw std::invalid_argument(
            "SPIR-V reflection requires bytecode and an entry point");
    }

    try
    {
        spirv_cross::Compiler compiler(spirv);
        const spv::ExecutionModel model = executionModel(stage);
        bool foundEntryPoint = false;
        for (const spirv_cross::EntryPoint& entry :
             compiler.get_entry_points_and_stages())
        {
            if (entry.name == entryPoint && entry.execution_model == model)
            {
                foundEntryPoint = true;
                break;
            }
        }
        if (!foundEntryPoint)
        {
            throw std::invalid_argument(
                "SPIR-V does not contain the requested stage entry point");
        }
        compiler.set_entry_point(entryPoint, model);

        ShaderInterface result{};
        result.stage = stage;
        result.entryPoint = entryPoint;
        const spirv_cross::ShaderResources resources =
            compiler.get_shader_resources();

        for (const spirv_cross::Resource& resource :
             resources.uniform_buffers)
        {
            appendResource(
                result,
                compiler,
                resource,
                ShaderResourceType::UniformBuffer);
            appendParameterBlock(result, compiler, resource);
        }
        for (const spirv_cross::Resource& resource :
             resources.storage_buffers)
        {
            appendResource(
                result,
                compiler,
                resource,
                ShaderResourceType::StorageBuffer);
        }
        for (const spirv_cross::Resource& resource :
             resources.separate_images)
        {
            appendResource(
                result,
                compiler,
                resource,
                ShaderResourceType::SampledImage);
        }
        for (const spirv_cross::Resource& resource :
             resources.separate_samplers)
        {
            appendResource(
                result,
                compiler,
                resource,
                ShaderResourceType::Sampler);
        }
        for (const spirv_cross::Resource& resource :
             resources.sampled_images)
        {
            appendResource(
                result,
                compiler,
                resource,
                ShaderResourceType::CombinedImageSampler);
        }
        for (const spirv_cross::Resource& resource : resources.stage_inputs)
        {
            appendStageIo(result.inputs, compiler, resource);
        }
        for (const spirv_cross::Resource& resource : resources.stage_outputs)
        {
            appendStageIo(result.outputs, compiler, resource);
        }
        for (const spirv_cross::Resource& resource :
             resources.push_constant_buffers)
        {
            const spirv_cross::SPIRType& type =
                compiler.get_type(resource.base_type_id);
            result.pushConstants.push_back({
                resourceName(compiler, resource),
                static_cast<uint32_t>(
                    compiler.get_declared_struct_size(type)),
                stage
            });
        }

        result.signature = interfaceSignature(result);
        return result;
    }
    catch (const spirv_cross::CompilerError& error)
    {
        throw std::invalid_argument(
            std::string("SPIR-V reflection failed: ") + error.what());
    }
}

} // namespace VkRenderer
