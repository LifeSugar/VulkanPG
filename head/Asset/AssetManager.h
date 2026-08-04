#pragma once

#include "Asset/AssetRegistry.h"
#include "Asset/MaterialAsset.h"
#include "Asset/MaterialTemplateAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/ModelAsset.h"
#include "Asset/ShaderAsset.h"
#include "Asset/TextureAsset.h"

namespace VkRenderer
{

/// Owns validated CPU assets without knowing how their CreateInfo was produced.
class AssetManager final
{
public:
    [[nodiscard]] TextureAssetHandle createTexture(
        TextureAsset::CreateInfo createInfo);
    [[nodiscard]] MaterialTemplateAssetHandle createMaterialTemplate(
        MaterialTemplateAsset::CreateInfo createInfo);
    [[nodiscard]] MaterialAssetHandle createMaterial(
        MaterialAsset::CreateInfo createInfo);
    [[nodiscard]] MeshAssetHandle createMesh(
        MeshAsset::CreateInfo createInfo);
    [[nodiscard]] ShaderAssetHandle createShader(
        ShaderAsset::CreateInfo createInfo);
    [[nodiscard]] ModelAssetHandle createModel(
        ModelAsset::CreateInfo createInfo);

    [[nodiscard]] const TextureAsset& texture(TextureAssetHandle handle) const;
    [[nodiscard]] const MaterialTemplateAsset& materialTemplate(
        MaterialTemplateAssetHandle handle) const;
    [[nodiscard]] const MaterialAsset& material(MaterialAssetHandle handle) const;
    [[nodiscard]] const MeshAsset& mesh(MeshAssetHandle handle) const;
    [[nodiscard]] const ShaderAsset& shader(ShaderAssetHandle handle) const;
    [[nodiscard]] const ModelAsset& model(ModelAssetHandle handle) const;

    [[nodiscard]] bool contains(TextureAssetHandle handle) const noexcept;
    [[nodiscard]] bool contains(MaterialTemplateAssetHandle handle) const noexcept;
    [[nodiscard]] bool contains(MaterialAssetHandle handle) const noexcept;
    [[nodiscard]] bool contains(MeshAssetHandle handle) const noexcept;
    [[nodiscard]] bool contains(ShaderAssetHandle handle) const noexcept;
    [[nodiscard]] bool contains(ModelAssetHandle handle) const noexcept;

    void reset() noexcept;

private:
    AssetRegistry<TextureAsset> textures_;
    AssetRegistry<MaterialTemplateAsset> materialTemplates_;
    AssetRegistry<MaterialAsset> materials_;
    AssetRegistry<MeshAsset> meshes_;
    AssetRegistry<ShaderAsset> shaders_;
    AssetRegistry<ModelAsset> models_;
};

} // namespace VkRenderer
