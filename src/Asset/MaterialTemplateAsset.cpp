#include "Asset/MaterialTemplateAsset.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace VkRenderer
{

MaterialTemplateAsset::MaterialTemplateAsset(CreateInfo createInfo)
{
    create(std::move(createInfo));
}

void MaterialTemplateAsset::create(CreateInfo createInfo)
{
    std::unordered_set<std::string> names;
    uint32_t requiredParameterBytes = 0;
    for (const MaterialParameterDesc& parameter : createInfo.parameters)
    {
        if (parameter.name.empty() || !names.insert(parameter.name).second)
        {
            throw std::invalid_argument(
                "material template parameter names must be non-empty and unique");
        }

        const uint32_t size = valueSize(parameter.type);
        if (parameter.byteOffset >
            std::numeric_limits<uint32_t>::max() - size)
        {
            throw std::overflow_error(
                "material template parameter range overflows uint32_t");
        }
        requiredParameterBytes = std::max(
            requiredParameterBytes,
            parameter.byteOffset + size);
    }

    for (std::size_t left = 0; left < createInfo.parameters.size(); ++left)
    {
        const MaterialParameterDesc& first = createInfo.parameters[left];
        const uint32_t firstEnd =
            first.byteOffset + valueSize(first.type);
        for (std::size_t right = left + 1;
             right < createInfo.parameters.size();
             ++right)
        {
            const MaterialParameterDesc& second = createInfo.parameters[right];
            const uint32_t secondEnd =
                second.byteOffset + valueSize(second.type);
            if (std::max(first.byteOffset, second.byteOffset) <
                std::min(firstEnd, secondEnd))
            {
                throw std::invalid_argument(
                    "material template parameter ranges overlap");
            }
        }
    }

    std::unordered_set<uint32_t> textureSlots;
    for (const MaterialTextureSlotDesc& texture : createInfo.textureSlots)
    {
        if (texture.slot == std::numeric_limits<uint32_t>::max() ||
            texture.name.empty() || !names.insert(texture.name).second ||
            !textureSlots.insert(texture.slot).second)
        {
            throw std::invalid_argument(
                "material template texture names and slots must be unique");
        }
    }

    if (createInfo.parameterDataSize == 0)
    {
        createInfo.parameterDataSize = requiredParameterBytes;
    }
    if (createInfo.parameterDataSize < requiredParameterBytes)
    {
        throw std::invalid_argument(
            "material template parameter buffer is smaller than its layout");
    }

    name_ = std::move(createInfo.name);
    shaders_ = std::move(createInfo.shaders);
    parameterDataSize_ = createInfo.parameterDataSize;
    parameters_ = std::move(createInfo.parameters);
    textureSlots_ = std::move(createInfo.textureSlots);
}

void MaterialTemplateAsset::reset() noexcept
{
    name_.clear();
    shaders_.clear();
    parameterDataSize_ = 0;
    parameters_.clear();
    textureSlots_.clear();
}

uint32_t MaterialTemplateAsset::valueSize(MaterialValueType type) noexcept
{
    switch (type)
    {
    case MaterialValueType::Float:
    case MaterialValueType::Int:
    case MaterialValueType::UInt:
    case MaterialValueType::Bool:
        return 4;
    case MaterialValueType::Float2:
        return 8;
    case MaterialValueType::Float3:
        return 12;
    case MaterialValueType::Float4:
        return 16;
    case MaterialValueType::Matrix4:
        return 64;
    }
    return 0;
}

} // namespace VkRenderer
