#pragma once

#include "vulkan/GraphicsPipeline.hpp"
#include "asset/AssetFwd.hpp"

namespace rubia::asset
{
class AssetManager;
}

namespace rubia::rhi::vulkan
{

[[nodiscard]] GraphicsPipeline::CreateInfo makeDefaultScenePipeline(
    const asset::AssetManager& assets,
    asset::ShaderProgramAssetHandle program,
    VkDescriptorSetLayout materialDescriptorSetLayout);

[[nodiscard]] GraphicsPipeline::CreateInfo makeDefaultPresentPipeline(
    const asset::AssetManager& assets,
    asset::ShaderProgramAssetHandle program);

} // namespace rubia::rhi::vulkan
