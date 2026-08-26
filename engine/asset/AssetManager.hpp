#pragma once

#include "asset/AssetRegistry.hpp"
#include "asset/MaterialAsset.hpp"
#include "asset/MaterialTemplateAsset.hpp"
#include "asset/MeshAsset.hpp"
#include "asset/ModelAsset.hpp"
#include "asset/ShaderAsset.hpp"
#include "asset/TextureAsset.hpp"
#include "asset/ValidationReport.hpp"

namespace VkRenderer
{

/// Owns validated CPU assets without knowing how their CreateInfo was produced.
class AssetManager final
{
public:
    [[nodiscard]] TextureAssetHandle createTexture(
        TextureAsset::CreateInfo createInfo);
    /// Replaces texture content while preserving references held by materials.
    [[nodiscard]] TextureAsset replaceTexture(
        TextureAssetHandle handle,
        TextureAsset replacement);
    [[nodiscard]] MaterialTemplateAssetHandle createMaterialTemplate(
        MaterialTemplateAsset::CreateInfo createInfo);
    [[nodiscard]] MaterialAssetHandle createMaterial(
        MaterialAsset::CreateInfo createInfo);
    [[nodiscard]] ValidationReport validateMaterialTemplate(
        const MaterialTemplateAsset::CreateInfo& createInfo) const;
    [[nodiscard]] ValidationReport validateMaterial(
        const MaterialAsset::CreateInfo& createInfo) const;
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
    [[nodiscard]] bool isMaterialTemplateCurrent(
        MaterialTemplateAssetHandle handle) const noexcept;

    [[nodiscard]] std::vector<TextureAssetHandle> textureHandles() const;
    [[nodiscard]] std::vector<MaterialAssetHandle> materialHandles() const;
    [[nodiscard]] std::vector<ModelAssetHandle> modelHandles() const;

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
