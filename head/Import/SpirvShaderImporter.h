#pragma once

#include "Asset/AssetManager.h"

#include <filesystem>
#include <string>

namespace VkRenderer
{

/// Imports one SPIR-V file into the source-independent ShaderAsset format.
class SpirvShaderImporter final
{
public:
    struct CreateInfo
    {
        AssetManager* assets = nullptr;
        std::filesystem::path path;
        std::string name;
        ShaderStage stage = ShaderStage::Vertex;
        std::string entryPoint = "main";
    };

    [[nodiscard]] ShaderAssetHandle import(
        const CreateInfo& createInfo) const;
};

} // namespace VkRenderer
