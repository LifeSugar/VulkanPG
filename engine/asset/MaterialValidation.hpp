#pragma once

#include "asset/MaterialAsset.hpp"
#include "asset/MaterialTemplateAsset.hpp"
#include "asset/ValidationReport.hpp"

#include <cstdint>
#include <vector>

namespace VkRenderer
{

class AssetManager;

[[nodiscard]] uint64_t calculateShaderInterfaceSignature(
    const std::vector<ShaderAssetHandle>& shaders,
    const AssetManager& assets);

[[nodiscard]] ValidationReport validateMaterialTemplateCreateInfo(
    const MaterialTemplateAsset::CreateInfo& createInfo,
    const AssetManager& assets);

[[nodiscard]] ValidationReport validateMaterialCreateInfo(
    const MaterialAsset::CreateInfo& createInfo,
    const AssetManager& assets);

} // namespace VkRenderer
