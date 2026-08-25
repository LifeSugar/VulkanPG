#pragma once

#include "Asset/ShaderInterface.h"

#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

/// CPU-only SPIR-V reflection. It does not create a Vulkan shader module.
class SpirvReflection final
{
public:
    [[nodiscard]] static ShaderInterface reflect(
        const std::vector<uint32_t>& spirv,
        ShaderStage stage,
        const std::string& entryPoint);
};

} // namespace VkRenderer
