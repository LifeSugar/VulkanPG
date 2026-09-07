#include "asset/ShaderAsset.hpp"

#include <stdexcept>
#include <utility>

namespace rubia::asset
{

ShaderAsset::ShaderAsset(CreateInfo createInfo)
{
    create(std::move(createInfo));
}

void ShaderAsset::create(CreateInfo createInfo)
{
    constexpr uint32_t kSpirvMagic = 0x07230203u;
    if (createInfo.interface.entryPoint.empty())
    {
        throw std::invalid_argument("shader entry point must not be empty");
    }
    if (createInfo.spirv.empty() || createInfo.spirv.front() != kSpirvMagic)
    {
        throw std::invalid_argument("shader bytecode is not valid SPIR-V");
    }
    if (createInfo.interface.interfaceHash == 0)
    {
        throw std::invalid_argument(
            "shader reflection interface hash is invalid");
    }

    name_ = std::move(createInfo.name);
    spirv_ = std::move(createInfo.spirv);
    interface_ = std::move(createInfo.interface);
}

void ShaderAsset::reset() noexcept
{
    name_.clear();
    spirv_.clear();
    interface_ = {};
}

} // namespace rubia::asset
