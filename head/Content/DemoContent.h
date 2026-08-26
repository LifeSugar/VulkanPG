#pragma once

#include "Asset/AssetFwd.h"

#include <filesystem>

namespace VkRenderer
{

class AssetManager;
class Scene;
class TextureImportRegistry;

/// Handles produced while loading the fixed demo content used by the runtime.
struct DemoContent
{
    TextureAssetHandle defaultTexture;
    TextureAssetHandle defaultDataTexture;
    TextureAssetHandle defaultNormalTexture;
    ShaderAssetHandle pbrVertexShader;
    ShaderAssetHandle pbrFragmentShader;
    ShaderAssetHandle presentVertexShader;
    ShaderAssetHandle presentFragmentShader;
    MaterialTemplateAssetHandle materialTemplate;
    MaterialAssetHandle defaultMaterial;
    ModelAssetHandle model;
};

/// Loads the existing built-in assets, demo model, and demo scene.
class DemoContentLoader final
{
public:
    struct CreateInfo
    {
        std::filesystem::path modelPath;
        std::filesystem::path pbrVertexShader;
        std::filesystem::path pbrFragmentShader;
        std::filesystem::path presentVertexShader;
        std::filesystem::path presentFragmentShader;
        std::filesystem::path cookedAssetDirectory;
    };

    [[nodiscard]] static DemoContent load(
        AssetManager& assets,
        Scene& scene,
        const CreateInfo& createInfo,
        TextureImportRegistry* textureImports = nullptr);
};

} // namespace VkRenderer
