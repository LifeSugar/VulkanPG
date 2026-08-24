#include "Asset/MaterialValidation.h"

#include "Asset/AssetManager.h"
#include "Asset/ShaderAsset.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace VkRenderer
{
namespace
{

[[nodiscard]] ShaderValueType shaderValueType(
    MaterialValueType type) noexcept
{
    switch (type)
    {
    case MaterialValueType::Float: return ShaderValueType::Float;
    case MaterialValueType::Float2: return ShaderValueType::Float2;
    case MaterialValueType::Float3: return ShaderValueType::Float3;
    case MaterialValueType::Float4: return ShaderValueType::Float4;
    case MaterialValueType::Matrix4: return ShaderValueType::Matrix4;
    case MaterialValueType::Int: return ShaderValueType::Int;
    case MaterialValueType::UInt: return ShaderValueType::UInt;
    case MaterialValueType::Bool: return ShaderValueType::Bool;
    }
    return ShaderValueType::Unknown;
}

[[nodiscard]] MaterialValueType materialValueType(
    const MaterialValue& value)
{
    switch (value.index())
    {
    case 0: return MaterialValueType::Float;
    case 1: return MaterialValueType::Float2;
    case 2: return MaterialValueType::Float3;
    case 3: return MaterialValueType::Float4;
    case 4: return MaterialValueType::Matrix4;
    case 5: return MaterialValueType::Int;
    case 6: return MaterialValueType::UInt;
    case 7: return MaterialValueType::Bool;
    default: return MaterialValueType::Float;
    }
}

[[nodiscard]] bool finiteMaterialValue(const MaterialValue& value)
{
    return std::visit(
        [](const auto& typedValue)
        {
            using Value = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_floating_point_v<Value>)
            {
                return std::isfinite(typedValue);
            }
            else if constexpr (
                std::is_same_v<Value, glm::vec2> ||
                std::is_same_v<Value, glm::vec3> ||
                std::is_same_v<Value, glm::vec4>)
            {
                for (glm::length_t index = 0;
                     index < typedValue.length();
                     ++index)
                {
                    if (!std::isfinite(typedValue[index]))
                    {
                        return false;
                    }
                }
                return true;
            }
            else if constexpr (std::is_same_v<Value, glm::mat4>)
            {
                for (glm::length_t column = 0; column < 4; ++column)
                {
                    for (glm::length_t row = 0; row < 4; ++row)
                    {
                        if (!std::isfinite(typedValue[column][row]))
                        {
                            return false;
                        }
                    }
                }
                return true;
            }
            else
            {
                return true;
            }
        },
        value);
}

[[nodiscard]] uint64_t bindingKey(uint32_t set, uint32_t binding) noexcept
{
    return (static_cast<uint64_t>(set) << 32u) | binding;
}

[[nodiscard]] const ShaderResourceDesc* findResource(
    const std::vector<const ShaderAsset*>& shaders,
    uint32_t set,
    uint32_t binding,
    ShaderResourceType type)
{
    for (const ShaderAsset* shader : shaders)
    {
        for (const ShaderResourceDesc& resource :
             shader->interface().resources)
        {
            if (resource.set == set && resource.binding == binding &&
                resource.type == type)
            {
                return &resource;
            }
        }
    }
    return nullptr;
}

[[nodiscard]] const ShaderParameterBlockDesc* findParameterBlock(
    const std::vector<const ShaderAsset*>& shaders,
    uint32_t set,
    uint32_t binding)
{
    for (const ShaderAsset* shader : shaders)
    {
        for (const ShaderParameterBlockDesc& block :
             shader->interface().parameterBlocks)
        {
            if (block.set == set && block.binding == binding)
            {
                return &block;
            }
        }
    }
    return nullptr;
}

[[nodiscard]] const ShaderBlockMemberDesc* findMember(
    const ShaderParameterBlockDesc& block,
    const std::string& name)
{
    const auto iterator = std::find_if(
        block.members.begin(),
        block.members.end(),
        [&](const ShaderBlockMemberDesc& member)
        {
            return member.name == name;
        });
    return iterator == block.members.end() ? nullptr : &*iterator;
}

[[nodiscard]] std::vector<const ShaderAsset*> resolveShaders(
    const std::vector<ShaderAssetHandle>& handles,
    const AssetManager& assets,
    ValidationReport& report)
{
    std::vector<const ShaderAsset*> result;
    result.reserve(handles.size());
    for (std::size_t index = 0; index < handles.size(); ++index)
    {
        if (!assets.contains(handles[index]))
        {
            report.addError(
                "Template.InvalidShaderHandle",
                "shaders[" + std::to_string(index) + "]",
                "shader handle is not owned by this AssetManager");
            continue;
        }
        result.push_back(&assets.shader(handles[index]));
    }
    return result;
}

void validateShaderStages(
    const std::vector<const ShaderAsset*>& shaders,
    ValidationReport& report)
{
    std::set<ShaderStage> stages;
    for (const ShaderAsset* shader : shaders)
    {
        if (!stages.insert(shader->stage()).second)
        {
            report.addError(
                "Template.DuplicateShaderStage",
                "shaders",
                "material template contains more than one shader for a stage");
        }
    }
    if (stages.count(ShaderStage::Vertex) == 0)
    {
        report.addError(
            "Template.MissingVertexShader",
            "shaders",
            "graphics material template requires a vertex shader");
    }
    if (stages.count(ShaderStage::Fragment) == 0)
    {
        report.addError(
            "Template.MissingFragmentShader",
            "shaders",
            "graphics material template requires a fragment shader");
    }
    if (stages.count(ShaderStage::Compute) != 0)
    {
        report.addError(
            "Template.ComputeShaderUnsupported",
            "shaders",
            "compute shaders cannot be part of a graphics material template");
    }
}

void validateCrossStageIo(
    const std::vector<const ShaderAsset*>& shaders,
    ValidationReport& report)
{
    const ShaderAsset* vertex = nullptr;
    const ShaderAsset* fragment = nullptr;
    for (const ShaderAsset* shader : shaders)
    {
        if (shader->stage() == ShaderStage::Vertex)
        {
            vertex = shader;
        }
        else if (shader->stage() == ShaderStage::Fragment)
        {
            fragment = shader;
        }
    }
    if (vertex == nullptr || fragment == nullptr)
    {
        return;
    }

    for (const ShaderStageIoDesc& input : fragment->interface().inputs)
    {
        const auto output = std::find_if(
            vertex->interface().outputs.begin(),
            vertex->interface().outputs.end(),
            [&](const ShaderStageIoDesc& candidate)
            {
                return candidate.location == input.location;
            });
        if (output == vertex->interface().outputs.end())
        {
            report.addError(
                "Template.StageInputMissing",
                "fragment.inputs[" + std::to_string(input.location) + "]",
                "fragment input has no vertex output at the same location");
        }
        else if (output->type != input.type)
        {
            report.addError(
                "Template.StageIoTypeMismatch",
                "fragment.inputs[" + std::to_string(input.location) + "]",
                "vertex output and fragment input types differ");
        }
    }
}

void validateResourceCompatibility(
    const std::vector<const ShaderAsset*>& shaders,
    ValidationReport& report)
{
    struct ResourceShape
    {
        ShaderResourceType type;
        uint32_t arrayCount;
    };
    std::unordered_map<uint64_t, ResourceShape> bindings;
    for (const ShaderAsset* shader : shaders)
    {
        for (const ShaderResourceDesc& resource :
             shader->interface().resources)
        {
            const uint64_t key = bindingKey(resource.set, resource.binding);
            const auto existing = bindings.find(key);
            if (existing == bindings.end())
            {
                bindings.emplace(
                    key,
                    ResourceShape{resource.type, resource.arrayCount});
            }
            else if (existing->second.type != resource.type ||
                     existing->second.arrayCount != resource.arrayCount)
            {
                report.addError(
                    "Template.CrossStageResourceMismatch",
                    "set " + std::to_string(resource.set) + " binding " +
                        std::to_string(resource.binding),
                    "shader stages declare incompatible descriptor resources");
            }
        }
    }
}

void validateRenderState(
    const MaterialRenderState& state,
    ValidationReport& report)
{
    switch (state.surfaceType)
    {
    case MaterialSurfaceType::Opaque:
    case MaterialSurfaceType::Transparent:
        break;
    default:
        report.addError(
            "Material.InvalidSurfaceType",
            "renderState.surfaceType",
            "material surface type is invalid");
    }
    switch (state.depth.compare)
    {
    case DepthCompare::Never:
    case DepthCompare::Less:
    case DepthCompare::Equal:
    case DepthCompare::LessEqual:
    case DepthCompare::Greater:
    case DepthCompare::NotEqual:
    case DepthCompare::GreaterEqual:
    case DepthCompare::Always:
        break;
    default:
        report.addError(
            "Material.InvalidDepthCompare",
            "renderState.depth.compare",
            "material depth comparison operation is invalid");
    }
    if (!std::isfinite(state.alphaClipThreshold) ||
        state.alphaClipThreshold < 0.0f ||
        state.alphaClipThreshold > 1.0f)
    {
        report.addError(
            "Material.InvalidAlphaClipThreshold",
            "renderState.alphaClipThreshold",
            "alpha clip threshold must be finite and within [0, 1]");
    }
    if (state.alphaClipEnabled && state.transparent())
    {
        report.addError(
            "Material.InvalidAlphaMode",
            "renderState",
            "alpha clipping currently requires an opaque material");
    }
    if (state.depth.writeEnabled && !state.depth.testEnabled)
    {
        report.addError(
            "Material.InvalidDepthState",
            "renderState.depth",
            "depth writes require depth testing");
    }
}

} // namespace

uint64_t calculateShaderInterfaceSignature(
    const std::vector<ShaderAssetHandle>& shaders,
    const AssetManager& assets)
{
    std::vector<std::pair<ShaderStage, uint64_t>> signatures;
    signatures.reserve(shaders.size());
    for (ShaderAssetHandle handle : shaders)
    {
        if (!assets.contains(handle))
        {
            return 0;
        }
        const ShaderAsset& shader = assets.shader(handle);
        signatures.emplace_back(
            shader.stage(),
            shader.interface().signature);
    }
    std::sort(signatures.begin(), signatures.end());

    uint64_t hash = UINT64_C(14695981039346656037);
    constexpr uint64_t kFnvPrime = UINT64_C(1099511628211);
    for (const auto& signature : signatures)
    {
        const uint64_t values[] = {
            static_cast<uint64_t>(signature.first),
            signature.second
        };
        for (uint64_t value : values)
        {
            for (uint32_t byte = 0; byte < 8; ++byte)
            {
                hash ^= static_cast<uint8_t>(value >> (byte * 8u));
                hash *= kFnvPrime;
            }
        }
    }
    return signatures.empty() ? 0 : hash;
}

ValidationReport validateMaterialTemplateCreateInfo(
    const MaterialTemplateAsset::CreateInfo& createInfo,
    const AssetManager& assets)
{
    ValidationReport report;
    if (createInfo.name.empty())
    {
        report.addWarning(
            "Template.EmptyName",
            "name",
            "material template has no display name");
    }
    if (createInfo.shaders.empty())
    {
        report.addError(
            "Template.NoShaders",
            "shaders",
            "material template requires shader assets");
    }

    const std::vector<const ShaderAsset*> shaders =
        resolveShaders(createInfo.shaders, assets, report);
    validateShaderStages(shaders, report);
    validateCrossStageIo(shaders, report);
    validateResourceCompatibility(shaders, report);

    const MaterialDescriptorBinding parameterBinding =
        createInfo.parameterBlock.descriptor;
    if (parameterBinding.binding == kInvalidShaderBinding)
    {
        report.addError(
            "Template.InvalidParameterBinding",
            "parameterBlock",
            "material parameter block binding is invalid");
    }

    const ShaderParameterBlockDesc* reflectedBlock = findParameterBlock(
        shaders,
        parameterBinding.set,
        parameterBinding.binding);
    if (reflectedBlock == nullptr)
    {
        report.addError(
            "Template.ParameterBlockMissing",
            "parameterBlock",
            "shaders do not declare the material parameter block");
    }

    std::unordered_set<std::string> parameterNames;
    std::vector<std::pair<uint32_t, uint32_t>> parameterRanges;
    uint32_t requiredBytes = 0;
    for (const MaterialParameterDesc& parameter : createInfo.parameters)
    {
        const std::string path = "parameters." + parameter.name;
        if (parameter.name.empty() ||
            !parameterNames.insert(parameter.name).second)
        {
            report.addError(
                "Template.InvalidParameterName",
                path,
                "parameter names must be non-empty and unique");
            continue;
        }
        const uint32_t size = MaterialTemplateAsset::valueSize(parameter.type);
        if (parameter.byteOffset >
            std::numeric_limits<uint32_t>::max() - size)
        {
            report.addError(
                "Template.ParameterRangeOverflow",
                path,
                "parameter byte range overflows uint32_t");
            continue;
        }
        requiredBytes = std::max(
            requiredBytes,
            parameter.byteOffset + size);
        const uint32_t rangeEnd = parameter.byteOffset + size;
        for (const auto& range : parameterRanges)
        {
            if (std::max(parameter.byteOffset, range.first) <
                std::min(rangeEnd, range.second))
            {
                report.addError(
                    "Template.ParameterRangeOverlap",
                    path,
                    "material parameter byte ranges overlap");
                break;
            }
        }
        parameterRanges.emplace_back(parameter.byteOffset, rangeEnd);

        if (reflectedBlock != nullptr)
        {
            const ShaderBlockMemberDesc* member =
                findMember(*reflectedBlock, parameter.name);
            if (member == nullptr)
            {
                report.addError(
                    "Template.ParameterMissing",
                    path,
                    "shader parameter block has no member with this name");
            }
            else
            {
                if (member->type != shaderValueType(parameter.type))
                {
                    report.addError(
                        "Template.ParameterTypeMismatch",
                        path,
                        "template and shader parameter types differ");
                }
                if (member->offset != parameter.byteOffset)
                {
                    report.addError(
                        "Template.ParameterOffsetMismatch",
                        path,
                        "template and shader parameter byte offsets differ");
                }
                if (member->size != size)
                {
                    report.addError(
                        "Template.ParameterSizeMismatch",
                        path,
                        "template and shader parameter byte sizes differ");
                }
            }
        }
    }

    if (reflectedBlock != nullptr)
    {
        for (const ShaderBlockMemberDesc& member : reflectedBlock->members)
        {
            if (parameterNames.count(member.name) == 0)
            {
                report.addError(
                    "Template.ShaderParameterUndeclared",
                    "parameterBlock." + member.name,
                    "shader material parameter is absent from the template");
            }
        }
        const uint32_t templateSize = createInfo.parameterDataSize == 0
            ? requiredBytes
            : createInfo.parameterDataSize;
        if (templateSize != reflectedBlock->byteSize)
        {
            report.addError(
                "Template.ParameterBlockSizeMismatch",
                "parameterDataSize",
                "template and shader parameter block sizes differ");
        }
    }
    if (createInfo.parameterDataSize != 0 &&
        createInfo.parameterDataSize < requiredBytes)
    {
        report.addError(
            "Template.ParameterDataTooSmall",
            "parameterDataSize",
            "parameter data buffer cannot contain all declared parameters");
    }

    std::unordered_set<std::string> textureNames;
    std::unordered_set<uint32_t> textureSlots;
    std::unordered_set<uint64_t> declaredImageBindings;
    std::unordered_set<uint64_t> declaredSamplerBindings;
    std::unordered_set<uint64_t> allDescriptorBindings;
    allDescriptorBindings.insert(bindingKey(
        parameterBinding.set,
        parameterBinding.binding));
    for (const MaterialTextureSlotDesc& slot : createInfo.textureSlots)
    {
        const std::string path = "textureSlots." + slot.name;
        if (slot.name.empty() || !textureNames.insert(slot.name).second)
        {
            report.addError(
                "Template.InvalidTextureName",
                path,
                "texture slot names must be non-empty and unique");
        }
        if (slot.slot == std::numeric_limits<uint32_t>::max() ||
            !textureSlots.insert(slot.slot).second)
        {
            report.addError(
                "Template.DuplicateTextureSlot",
                path,
                "texture slot indices must be unique");
        }

        const uint64_t imageKey = bindingKey(
            slot.imageBinding.set,
            slot.imageBinding.binding);
        const uint64_t samplerKey = bindingKey(
            slot.samplerBinding.set,
            slot.samplerBinding.binding);
        if (slot.imageBinding.binding == kInvalidShaderBinding ||
            !declaredImageBindings.insert(imageKey).second ||
            !allDescriptorBindings.insert(imageKey).second)
        {
            report.addError(
                "Template.InvalidImageBinding",
                path,
                "sampled-image bindings must be valid and unique");
        }
        if (slot.samplerBinding.binding == kInvalidShaderBinding ||
            !declaredSamplerBindings.insert(samplerKey).second ||
            !allDescriptorBindings.insert(samplerKey).second)
        {
            report.addError(
                "Template.InvalidSamplerBinding",
                path,
                "sampler bindings must be valid and unique");
        }
        if (slot.imageBinding.set != parameterBinding.set ||
            slot.samplerBinding.set != parameterBinding.set)
        {
            report.addError(
                "Template.MaterialSetMismatch",
                path,
                "parameter, image, and sampler bindings must use one material descriptor set");
        }

        if (findResource(
                shaders,
                slot.imageBinding.set,
                slot.imageBinding.binding,
                ShaderResourceType::SampledImage) == nullptr)
        {
            report.addWarning(
                "Template.InactiveImageBinding",
                path,
                "current shader entry points do not use this sampled image binding");
        }
        if (findResource(
                shaders,
                slot.samplerBinding.set,
                slot.samplerBinding.binding,
                ShaderResourceType::Sampler) == nullptr)
        {
            report.addWarning(
                "Template.InactiveSamplerBinding",
                path,
                "current shader entry points do not use this sampler binding");
        }
    }

    for (const ShaderAsset* shader : shaders)
    {
        for (const ShaderResourceDesc& resource :
             shader->interface().resources)
        {
            if (resource.set != parameterBinding.set)
            {
                continue;
            }
            const uint64_t key = bindingKey(resource.set, resource.binding);
            if (resource.type == ShaderResourceType::SampledImage &&
                declaredImageBindings.count(key) == 0)
            {
                report.addError(
                    "Template.ShaderImageUndeclared",
                    "set " + std::to_string(resource.set) + " binding " +
                        std::to_string(resource.binding),
                    "shader sampled image is absent from the template");
            }
            else if (resource.type == ShaderResourceType::Sampler &&
                     declaredSamplerBindings.count(key) == 0)
            {
                report.addError(
                    "Template.ShaderSamplerUndeclared",
                    "set " + std::to_string(resource.set) + " binding " +
                        std::to_string(resource.binding),
                    "shader sampler is absent from the template");
            }
        }
    }
    return report;
}

ValidationReport validateMaterialCreateInfo(
    const MaterialAsset::CreateInfo& createInfo,
    const AssetManager& assets)
{
    ValidationReport report;
    validateRenderState(createInfo.renderState, report);
    if (!assets.contains(createInfo.materialTemplate))
    {
        report.addError(
            "Material.InvalidTemplateHandle",
            "materialTemplate",
            "material template is not owned by this AssetManager");
        return report;
    }

    const MaterialTemplateAsset& materialTemplate =
        assets.materialTemplate(createInfo.materialTemplate);
    if (calculateShaderInterfaceSignature(
            materialTemplate.shaders(),
            assets) != materialTemplate.shaderInterfaceSignature())
    {
        report.addError(
            "Material.StaleTemplateInterface",
            "materialTemplate",
            "material template shader interface signature is stale");
    }

    std::vector<bool> assignedParameters(
        materialTemplate.parameters().size(),
        false);
    for (const MaterialParameterAssignment& assignment :
         createInfo.parameters)
    {
        const auto iterator = std::find_if(
            materialTemplate.parameters().begin(),
            materialTemplate.parameters().end(),
            [&](const MaterialParameterDesc& parameter)
            {
                return parameter.name == assignment.name;
            });
        const std::string path = "parameters." + assignment.name;
        if (iterator == materialTemplate.parameters().end())
        {
            report.addError(
                "Material.UnknownParameter",
                path,
                "parameter is absent from the material template");
            continue;
        }
        const std::size_t index = static_cast<std::size_t>(
            iterator - materialTemplate.parameters().begin());
        if (assignedParameters[index])
        {
            report.addError(
                "Material.DuplicateParameter",
                path,
                "parameter is assigned more than once");
        }
        assignedParameters[index] = true;
        if (materialValueType(assignment.value) != iterator->type)
        {
            report.addError(
                "Material.ParameterTypeMismatch",
                path,
                "assigned value type does not match the template");
        }
        if (!finiteMaterialValue(assignment.value))
        {
            report.addError(
                "Material.NonFiniteParameter",
                path,
                "floating-point material values must be finite");
        }
    }
    for (std::size_t index = 0;
         index < materialTemplate.parameters().size();
         ++index)
    {
        if (materialTemplate.parameters()[index].required &&
            !assignedParameters[index])
        {
            report.addError(
                "Material.RequiredParameterMissing",
                "parameters." + materialTemplate.parameters()[index].name,
                "required material parameter is not assigned");
        }
    }

    uint32_t textureCount = 0;
    for (const MaterialTextureSlotDesc& slot :
         materialTemplate.textureSlots())
    {
        textureCount = std::max(textureCount, slot.slot + 1);
    }
    std::vector<bool> assignedTextures(textureCount, false);
    for (const MaterialTextureAssignment& assignment : createInfo.textures)
    {
        const auto iterator = std::find_if(
            materialTemplate.textureSlots().begin(),
            materialTemplate.textureSlots().end(),
            [&](const MaterialTextureSlotDesc& slot)
            {
                return slot.name == assignment.name;
            });
        const std::string path = "textures." + assignment.name;
        if (iterator == materialTemplate.textureSlots().end())
        {
            report.addError(
                "Material.UnknownTextureSlot",
                path,
                "texture slot is absent from the material template");
            continue;
        }
        if (!assets.contains(assignment.texture))
        {
            report.addError(
                "Material.InvalidTextureHandle",
                path,
                "texture handle is not owned by this AssetManager");
        }
        if (assignedTextures[iterator->slot])
        {
            report.addError(
                "Material.DuplicateTexture",
                path,
                "texture slot is assigned more than once");
        }
        assignedTextures[iterator->slot] = true;
    }
    for (const MaterialTextureSlotDesc& slot :
         materialTemplate.textureSlots())
    {
        if (slot.required && !assignedTextures[slot.slot])
        {
            report.addError(
                "Material.RequiredTextureMissing",
                "textures." + slot.name,
                "required texture slot is not assigned");
        }
    }
    return report;
}

} // namespace VkRenderer
