#include "Asset/ShaderAsset.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

ShaderAsset::ShaderAsset(CreateInfo createInfo)
{
    create(std::move(createInfo));
}

void ShaderAsset::create(CreateInfo createInfo)
{
    constexpr uint32_t kSpirvMagic = 0x07230203u;
    if (createInfo.entryPoint.empty())
    {
        throw std::invalid_argument("shader entry point must not be empty");
    }
    if (createInfo.spirv.empty() || createInfo.spirv.front() != kSpirvMagic)
    {
        throw std::invalid_argument("shader bytecode is not valid SPIR-V");
    }

    name_ = std::move(createInfo.name);
    stage_ = createInfo.stage;
    entryPoint_ = std::move(createInfo.entryPoint);
    spirv_ = std::move(createInfo.spirv);
}

void ShaderAsset::reset() noexcept
{
    name_.clear();
    stage_ = ShaderStage::Vertex;
    entryPoint_ = "main";
    spirv_.clear();
}

} // namespace VkRenderer
