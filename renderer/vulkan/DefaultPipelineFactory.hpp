#pragma once

#include "vulkan/GraphicsPipeline.hpp"

namespace rubia::asset
{
class ShaderAsset;
}

namespace rubia::rhi::vulkan
{

[[nodiscard]] GraphicsPipeline::CreateInfo makeDefaultScenePipeline(
    const asset::ShaderAsset& vertexShader,
    const asset::ShaderAsset& fragmentShader,
    VkDescriptorSetLayout materialDescriptorSetLayout);

[[nodiscard]] GraphicsPipeline::CreateInfo makeDefaultPresentPipeline(
    const asset::ShaderAsset& vertexShader,
    const asset::ShaderAsset& fragmentShader);

} // namespace rubia::rhi::vulkan
