#include "asset/ShaderAsset.hpp"

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
    if (createInfo.interface.signature == 0 ||
        createInfo.interface.stage != createInfo.stage ||
        createInfo.interface.entryPoint != createInfo.entryPoint)
    {
        throw std::invalid_argument(
            "shader reflection does not match its stage and entry point");
    }

    name_ = std::move(createInfo.name);
    stage_ = createInfo.stage;
    entryPoint_ = std::move(createInfo.entryPoint);
    spirv_ = std::move(createInfo.spirv);
    interface_ = std::move(createInfo.interface);
}

void ShaderAsset::reset() noexcept
{
    name_.clear();
    stage_ = ShaderStage::Vertex;
    entryPoint_ = "main";
    spirv_.clear();
    interface_ = {};
}

} // namespace VkRenderer
