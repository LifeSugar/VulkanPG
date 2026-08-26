#pragma once

#include "vulkan/GraphicsPipeline.hpp"

namespace VkRenderer
{

class ShaderAsset;

[[nodiscard]] GraphicsPipeline::CreateInfo makeDefaultScenePipeline(
    const ShaderAsset& vertexShader,
    const ShaderAsset& fragmentShader,
    VkDescriptorSetLayout materialDescriptorSetLayout);

[[nodiscard]] GraphicsPipeline::CreateInfo makeDefaultPresentPipeline(
    const ShaderAsset& vertexShader,
    const ShaderAsset& fragmentShader);

} // namespace VkRenderer
