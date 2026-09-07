#include "shader/SpirvReflection.hpp"

#include <spirv_cross.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace rubia::importer::shader
{
namespace
{

[[nodiscard]] spv::ExecutionModel executionModel(asset::ShaderStage stage)
{
    switch (stage)
    {
    case asset::ShaderStage::Vertex: return spv::ExecutionModelVertex;
    case asset::ShaderStage::Fragment: return spv::ExecutionModelFragment;
    case asset::ShaderStage::Compute: return spv::ExecutionModelGLCompute;
    }
    throw std::invalid_argument("unsupported shader stage");
}

[[nodiscard]] asset::ShaderDataType dataType(
    const spirv_cross::SPIRType& type) noexcept
{
    using BaseType = spirv_cross::SPIRType::BaseType;
    if (type.columns == 4 && type.vecsize == 4 &&
        type.basetype == BaseType::Float)
    {
        return asset::ShaderDataType::Matrix4;
    }
    if (type.columns != 1)
    {
        return asset::ShaderDataType::Unknown;
    }

    if (type.basetype == BaseType::Float)
    {
        switch (type.vecsize)
        {
        case 1: return asset::ShaderDataType::Float;
        case 2: return asset::ShaderDataType::Float2;
        case 3: return asset::ShaderDataType::Float3;
        case 4: return asset::ShaderDataType::Float4;
        default: return asset::ShaderDataType::Unknown;
        }
    }
    if (type.vecsize != 1)
    {
        return asset::ShaderDataType::Unknown;
    }
    switch (type.basetype)
    {
    case BaseType::Int: return asset::ShaderDataType::Int;
    case BaseType::UInt: return asset::ShaderDataType::UInt;
    case BaseType::Boolean: return asset::ShaderDataType::Bool;
    default: return asset::ShaderDataType::Unknown;
    }
}

[[nodiscard]] bool isRuntimeArray(
    const spirv_cross::SPIRType& type) noexcept
{
    return std::find(type.array.begin(), type.array.end(), 0u) !=
        type.array.end();
}

[[nodiscard]] uint32_t checkedByteSize(
    std::size_t value,
    const char* description)
{
    if (value > std::numeric_limits<uint32_t>::max())
    {
        throw std::overflow_error(
            std::string(description) + " exceeds uint32_t");
    }
    return static_cast<uint32_t>(value);
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

[[nodiscard]] asset::ShaderBufferLayoutDesc reflectBufferLayout(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::Resource& resource)
{
    const spirv_cross::SPIRType& blockType =
        compiler.get_type(resource.base_type_id);
    asset::ShaderBufferLayoutDesc layout{};
    layout.members.reserve(blockType.member_types.size());

    bool hasRuntimeArray = false;
    for (uint32_t memberIndex = 0;
         memberIndex < static_cast<uint32_t>(blockType.member_types.size());
         ++memberIndex)
    {
        const spirv_cross::SPIRType& memberType =
            compiler.get_type(blockType.member_types[memberIndex]);
        const bool runtimeArray = isRuntimeArray(memberType);
        hasRuntimeArray = hasRuntimeArray || runtimeArray;

        asset::ShaderStructMemberDesc member{};
        member.name = compiler.get_member_name(
            resource.base_type_id,
            memberIndex);
        member.type = dataType(memberType);
        member.offset = compiler.type_struct_member_offset(
            blockType,
            memberIndex);
        member.size = runtimeArray
            ? 0
            : checkedByteSize(
                  compiler.get_declared_struct_member_size(
                      blockType,
                      memberIndex),
                  "shader block member size");
        member.arrayCount = arrayCount(memberType);
        member.runtimeArray = runtimeArray;
        member.arrayStride = memberType.array.empty()
            ? 0
            : compiler.type_struct_member_array_stride(
                  blockType,
                  memberIndex);
        member.matrixStride = memberType.columns <= 1
            ? 0
            : compiler.type_struct_member_matrix_stride(
                  blockType,
                  memberIndex);
        member.rowMajor = memberType.columns > 1 &&
            compiler.has_member_decoration(
                resource.base_type_id,
                memberIndex,
                spv::DecorationRowMajor);
        layout.members.push_back(std::move(member));
    }

    if (hasRuntimeArray &&
        (layout.members.empty() || !layout.members.back().runtimeArray))
    {
        throw std::invalid_argument(
            "shader buffer runtime array must be the final block member");
    }
    // SPIRV-Cross reports zero for the ordinary declared size of a runtime
    // SSBO. Asking for zero runtime elements gives the fixed ABI prefix size.
    layout.minimumByteSize = checkedByteSize(
        hasRuntimeArray
            ? compiler.get_declared_struct_size_runtime_array(blockType, 0)
            : compiler.get_declared_struct_size(blockType),
        "shader buffer block size");
    return layout;
}

void appendDescriptorBinding(
    asset::ShaderInterface& result,
    const spirv_cross::Compiler& compiler,
    const spirv_cross::Resource& resource,
    asset::ShaderDescriptorType descriptorType,
    bool includeBufferLayout)
{
    const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
    asset::ShaderDescriptorBindingDesc descriptor{};
    descriptor.name = resourceName(compiler, resource);
    descriptor.set = compiler.get_decoration(
        resource.id,
        spv::DecorationDescriptorSet);
    descriptor.binding = compiler.get_decoration(
        resource.id,
        spv::DecorationBinding);
    descriptor.type = descriptorType;
    if (descriptorType == asset::ShaderDescriptorType::StorageBuffer)
    {
        // Access decorations may live on either the block variable or its
        // members; get_buffer_block_flags() deliberately merges both forms.
        const spirv_cross::Bitset flags =
            compiler.get_buffer_block_flags(resource.id);
        const bool readable = !flags.get(spv::DecorationNonReadable);
        const bool writable = !flags.get(spv::DecorationNonWritable);
        descriptor.access = readable && writable
            ? asset::ShaderResourceAccess::ReadWrite
            : writable
                ? asset::ShaderResourceAccess::WriteOnly
                : asset::ShaderResourceAccess::ReadOnly;
    }
    descriptor.arrayCount = arrayCount(type);
    if (includeBufferLayout)
    {
        descriptor.bufferLayout = reflectBufferLayout(compiler, resource);
    }
    result.descriptorBindings.push_back(std::move(descriptor));
}

void appendStageVariable(
    std::vector<asset::ShaderStageVariableDesc>& destination,
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
        dataType(compiler.get_type(resource.type_id))
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

void hashBufferLayout(
    uint64_t& hash,
    const asset::ShaderBufferLayoutDesc& layout) noexcept
{
    hashValue(hash, layout.minimumByteSize);
    const uint64_t memberCount = layout.members.size();
    hashValue(hash, memberCount);
    for (const asset::ShaderStructMemberDesc& member : layout.members)
    {
        hashString(hash, member.name);
        hashValue(hash, member.type);
        hashValue(hash, member.offset);
        hashValue(hash, member.size);
        hashValue(hash, member.arrayCount);
        hashValue(hash, member.runtimeArray);
        hashValue(hash, member.arrayStride);
        hashValue(hash, member.matrixStride);
        hashValue(hash, member.rowMajor);
    }
}

void hashStageVariables(
    uint64_t& hash,
    std::vector<asset::ShaderStageVariableDesc> variables)
{
    std::sort(
        variables.begin(),
        variables.end(),
        [](const auto& left, const auto& right)
        {
            return std::tie(left.location, left.type, left.name) <
                std::tie(right.location, right.type, right.name);
        });
    const uint64_t variableCount = variables.size();
    hashValue(hash, variableCount);
    for (const asset::ShaderStageVariableDesc& variable : variables)
    {
        hashString(hash, variable.name);
        hashValue(hash, variable.location);
        hashValue(hash, variable.type);
    }
}

[[nodiscard]] uint64_t interfaceHash(const asset::ShaderInterface& interface)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    hashValue(hash, interface.stage);
    hashString(hash, interface.entryPoint);

    std::vector<asset::ShaderDescriptorBindingDesc> descriptors =
        interface.descriptorBindings;
    // Reflection enumeration order is not part of the ABI. Canonicalize every
    // unordered collection before hashing so equivalent modules hash equally.
    std::sort(
        descriptors.begin(),
        descriptors.end(),
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
    const uint64_t descriptorCount = descriptors.size();
    hashValue(hash, descriptorCount);
    for (const asset::ShaderDescriptorBindingDesc& descriptor : descriptors)
    {
        hashString(hash, descriptor.name);
        hashValue(hash, descriptor.set);
        hashValue(hash, descriptor.binding);
        hashValue(hash, descriptor.type);
        hashValue(hash, descriptor.access);
        hashValue(hash, descriptor.arrayCount);
        const bool hasBufferLayout = descriptor.bufferLayout.has_value();
        hashValue(hash, hasBufferLayout);
        if (descriptor.bufferLayout)
        {
            hashBufferLayout(hash, *descriptor.bufferLayout);
        }
    }

    hashStageVariables(hash, interface.inputs);
    hashStageVariables(hash, interface.outputs);

    std::vector<asset::ShaderPushConstantBlockDesc> blocks =
        interface.pushConstantBlocks;
    std::sort(
        blocks.begin(),
        blocks.end(),
        [](const auto& left, const auto& right)
        {
            return left.name < right.name;
        });
    const uint64_t blockCount = blocks.size();
    hashValue(hash, blockCount);
    for (const asset::ShaderPushConstantBlockDesc& block : blocks)
    {
        hashString(hash, block.name);
        hashBufferLayout(hash, block.layout);
    }
    return hash;
}

} // namespace

asset::ShaderInterface SpirvReflection::reflect(
    const std::vector<uint32_t>& spirv,
    asset::ShaderStage stage,
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

        asset::ShaderInterface result{};
        result.stage = stage;
        result.entryPoint = entryPoint;
        const spirv_cross::ShaderResources resources =
            compiler.get_shader_resources();

        for (const spirv_cross::Resource& resource :
             resources.uniform_buffers)
        {
            appendDescriptorBinding(
                result,
                compiler,
                resource,
                asset::ShaderDescriptorType::UniformBuffer,
                true);
        }
        for (const spirv_cross::Resource& resource :
             resources.storage_buffers)
        {
            appendDescriptorBinding(
                result,
                compiler,
                resource,
                asset::ShaderDescriptorType::StorageBuffer,
                true);
        }
        for (const spirv_cross::Resource& resource :
             resources.separate_images)
        {
            appendDescriptorBinding(
                result,
                compiler,
                resource,
                asset::ShaderDescriptorType::SampledImage,
                false);
        }
        for (const spirv_cross::Resource& resource :
             resources.separate_samplers)
        {
            appendDescriptorBinding(
                result,
                compiler,
                resource,
                asset::ShaderDescriptorType::Sampler,
                false);
        }
        for (const spirv_cross::Resource& resource :
             resources.sampled_images)
        {
            appendDescriptorBinding(
                result,
                compiler,
                resource,
                asset::ShaderDescriptorType::CombinedImageSampler,
                false);
        }
        for (const spirv_cross::Resource& resource : resources.stage_inputs)
        {
            appendStageVariable(result.inputs, compiler, resource);
        }
        for (const spirv_cross::Resource& resource : resources.stage_outputs)
        {
            appendStageVariable(result.outputs, compiler, resource);
        }
        for (const spirv_cross::Resource& resource :
             resources.push_constant_buffers)
        {
            result.pushConstantBlocks.push_back({
                resourceName(compiler, resource),
                reflectBufferLayout(compiler, resource)
            });
        }

        result.interfaceHash = interfaceHash(result);
        return result;
    }
    catch (const spirv_cross::CompilerError& error)
    {
        throw std::invalid_argument(
            std::string("SPIR-V reflection failed: ") + error.what());
    }
}

} // namespace rubia::importer::shader
