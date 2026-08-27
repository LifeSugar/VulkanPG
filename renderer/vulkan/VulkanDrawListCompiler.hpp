#pragma once

#include "render/RenderList.hpp"
#include "vulkan/VulkanDrawList.hpp"

namespace rubia::rhi::vulkan
{

class RenderAssetCache;

/// Resolves backend-neutral draw intents against Vulkan GPU residency.
class VulkanDrawListCompiler final
{
public:
    [[nodiscard]] VulkanDrawList compile(
        const render::RenderList& source,
        const RenderAssetCache& resources) const;
};

} // namespace rubia::rhi::vulkan
