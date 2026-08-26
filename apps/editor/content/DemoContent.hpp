#pragma once

#include "asset/AssetFwd.hpp"
#include "texture/TextureImportSettings.hpp"

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
    /// Demo-only defaults applied to the editable import record after a glTF
    /// texture is decoded. The policy chooses settings; it does not cook the
    /// texture or change the generic glTF importer.
    struct TextureImportPolicy
    {
        KtxPayloadEncoding payloadEncoding = KtxPayloadEncoding::Uastc;
        bool generateMipmaps = false;
        TextureFormat colorTranscodeFormat = TextureFormat::BC7UNorm;
        TextureFormat dataTranscodeFormat = TextureFormat::BC7UNorm;
        TextureFormat normalTranscodeFormat = TextureFormat::BC5UNorm;
        bool highQualityTranscode = true;
    };

    struct CreateInfo
    {
        std::filesystem::path modelPath;
        std::filesystem::path pbrVertexShader;
        std::filesystem::path pbrFragmentShader;
        std::filesystem::path presentVertexShader;
        std::filesystem::path presentFragmentShader;
        std::filesystem::path cookedAssetDirectory;
        TextureImportPolicy textureImportPolicy;
    };

    [[nodiscard]] static DemoContent load(
        AssetManager& assets,
        Scene& scene,
        const CreateInfo& createInfo,
        TextureImportRegistry* textureImports = nullptr);
};

} // namespace VkRenderer
