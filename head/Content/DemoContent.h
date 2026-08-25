#pragma once

#include "Asset/AssetFwd.h"

namespace VkRenderer
{

class AssetManager;
class Scene;

/// Handles produced while loading the fixed demo content used by the runtime.
struct DemoContent
{
    TextureAssetHandle defaultTexture;
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
    [[nodiscard]] static DemoContent load(
        AssetManager& assets,
        Scene& scene);
};

} // namespace VkRenderer
