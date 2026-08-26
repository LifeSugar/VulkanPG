#pragma once

#include "vulkan/VulkanDrawList.hpp"

namespace VkRenderer
{

class RenderAssetCache;
struct RenderList;

/// Resolves backend-neutral draw intents against Vulkan GPU residency.
class VulkanDrawListCompiler final
{
public:
    [[nodiscard]] VulkanDrawList compile(
        const RenderList& source,
        const RenderAssetCache& resources) const;
};

} // namespace VkRenderer
