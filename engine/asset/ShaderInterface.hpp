#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rubia::asset
{

/// One selected SPIR-V entry point's execution stage.
enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute
};

/// Scalar, vector, or matrix shape understood by the engine's reflection
/// consumers. Unknown deliberately represents valid SPIR-V shapes that the
/// current material system cannot author directly, such as nested structs.
enum class ShaderDataType
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

/// Vulkan descriptor category reflected from a shader resource declaration.
/// This describes the descriptor itself; buffer member layout, when present,
/// is stored separately in ShaderDescriptorBindingDesc::bufferLayout.
enum class ShaderDescriptorType
{
    UniformBuffer,
    StorageBuffer,
    SampledImage,
    Sampler,
    CombinedImageSampler
};

/// Access permitted by the selected entry point through a descriptor. Uniform
/// buffers and sampled resources are ReadOnly; storage buffers may declare any
/// of these modes. This is shader semantics, not a Vulkan memory barrier.
enum class ShaderResourceAccess
{
    ReadOnly,
    WriteOnly,
    ReadWrite
};

/// One top-level member in a reflected UBO, SSBO, or push-constant block.
/// Offsets, sizes, and strides are the ABI values emitted into SPIR-V, not C++
/// sizeof/alignment guesses.
struct ShaderStructMemberDesc
{
    std::string name;
    ShaderDataType type = ShaderDataType::Unknown;
    uint32_t offset = 0;
    /// Complete declared byte size. A runtime-sized array has size zero.
    uint32_t size = 0;
    /// Product of all statically-sized array dimensions. Zero means the count
    /// depends on runtime or specialization data. Non-array members use one.
    uint32_t arrayCount = 1;
    /// Distinguishes an unsized trailing SSBO array from a specialization-sized
    /// array. Only an actual runtime array contributes to the block's runtime
    /// byte size.
    bool runtimeArray = false;
    /// Distance between array elements, or zero for a non-array member.
    uint32_t arrayStride = 0;
    /// Distance between adjacent major vectors, or zero for a non-matrix
    /// member. rowMajor determines whether those vectors are rows or columns.
    uint32_t matrixStride = 0;
    /// True when SPIR-V decorates the member as RowMajor. Matrix members are
    /// column-major otherwise; this flag is false for non-matrix members.
    bool rowMajor = false;

    [[nodiscard]] bool runtimeSized() const noexcept
    {
        return runtimeArray;
    }
};

/// Reflected memory layout shared by uniform buffers, storage buffers, and
/// push-constant blocks.
struct ShaderBufferLayoutDesc
{
    /// Full byte size for a fixed block. For a block ending in a runtime array,
    /// this is the minimum size with zero runtime elements. The runtime size is
    /// minimumByteSize + member.arrayStride * elementCount.
    uint32_t minimumByteSize = 0;
    std::vector<ShaderStructMemberDesc> members;

    [[nodiscard]] bool runtimeSized() const noexcept
    {
        return !members.empty() && members.back().runtimeSized();
    }
};

/// One descriptor binding used by the selected shader entry point.
/// Stage visibility is inherited from ShaderInterface::stage. After multiple
/// shader stages are merged, the future program interface should replace that
/// single stage with a stage mask.
struct ShaderDescriptorBindingDesc
{
    std::string name;
    uint32_t set = 0;
    uint32_t binding = 0;
    ShaderDescriptorType type = ShaderDescriptorType::UniformBuffer;
    ShaderResourceAccess access = ShaderResourceAccess::ReadOnly;
    /// Descriptor-array element count. Zero means the count is not statically
    /// known (for example, a runtime descriptor array).
    uint32_t arrayCount = 1;
    /// Present for UniformBuffer and StorageBuffer descriptors. Images and
    /// samplers do not have a buffer-member layout.
    std::optional<ShaderBufferLayoutDesc> bufferLayout;
};

/// One location-based input or output of the selected entry point.
struct ShaderStageVariableDesc
{
    std::string name;
    uint32_t location = 0;
    ShaderDataType type = ShaderDataType::Unknown;
};

/// One push-constant block. It is kept outside descriptorBindings because push
/// constants do not have a descriptor set or binding.
struct ShaderPushConstantBlockDesc
{
    std::string name;
    ShaderBufferLayoutDesc layout;
};

/// Renderer-facing ABI reflected from one selected SPIR-V entry point.
///
/// This is derived data owned by ShaderAsset: callers author SPIR-V plus an
/// entry-point request, and the shader importer produces this immutable
/// snapshot. It contains no Vulkan objects and no material semantics.
struct ShaderInterface
{
    ShaderStage stage = ShaderStage::Vertex;
    std::string entryPoint = "main";
    std::vector<ShaderDescriptorBindingDesc> descriptorBindings;
    std::vector<ShaderStageVariableDesc> inputs;
    std::vector<ShaderStageVariableDesc> outputs;
    std::vector<ShaderPushConstantBlockDesc> pushConstantBlocks;
    /// Stable fingerprint of every reflected field above. Shader bytecode that
    /// changes without changing its ABI may keep the same interface hash.
    uint64_t interfaceHash = 0;
};

} // namespace rubia::asset
