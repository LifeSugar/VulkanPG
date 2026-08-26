#pragma once

#include "asset/AssetManager.hpp"
#include "vulkan/DescriptorPool.hpp"
#include "vulkan/DescriptorSetLayout.hpp"
#include "vulkan/GpuMaterial.hpp"
#include "vulkan/GpuTexture.hpp"
#include "vulkan/Mesh.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace VkRenderer
{

/// Renderer-owned GPU representations reachable from one or more ModelAssets.
class RenderAssetCache final
{
public:
    RenderAssetCache() = default;
    ~RenderAssetCache();

    RenderAssetCache(const RenderAssetCache&) = delete;
    RenderAssetCache& operator=(const RenderAssetCache&) = delete;
    RenderAssetCache(RenderAssetCache&&) = delete;
    RenderAssetCache& operator=(RenderAssetCache&&) = delete;

    void create(
        const Device& device,
        UploadContext& uploadContext,
        const AssetManager& assets,
        const std::vector<ModelAssetHandle>& models);
    /// Builds a replacement without changing the live cache. This lets the
    /// caller finish disk/CPU validation before committing the GPU swap.
    [[nodiscard]] GpuTexture stageTextureReplacement(
        const Device& device,
        UploadContext& uploadContext,
        const TextureAsset& replacement) const;
    /// Commits a staged texture under the existing handle and rewrites every
    /// cached material descriptor that references it. The caller must ensure
    /// no submitted frame is using the old descriptors/resources.
    [[nodiscard]] GpuTexture commitTextureReplacement(
        const Device& device,
        const AssetManager& assets,
        TextureAssetHandle handle,
        GpuTexture replacement);
    void reset() noexcept;

    [[nodiscard]] const Mesh& mesh(MeshAssetHandle handle) const;
    [[nodiscard]] const Mesh* tryMesh(
        MeshAssetHandle handle) const noexcept;
    [[nodiscard]] const GpuMaterial& material(
        MaterialAssetHandle handle) const;
    [[nodiscard]] const GpuMaterial* tryMaterial(
        MaterialAssetHandle handle) const noexcept;
    /// Returns an uploaded texture for Editor previews and material binding.
    [[nodiscard]] const GpuTexture& texture(
        TextureAssetHandle handle) const;
    [[nodiscard]] const GpuTexture* tryTexture(
        TextureAssetHandle handle) const noexcept;
    [[nodiscard]] VkDescriptorSetLayout materialDescriptorSetLayout() const
        noexcept
    {
        return materialDescriptorSetLayout_.get();
    }
private:
    struct TextureEntry
    {
        uint32_t generation = 0;
        GpuTexture texture;
    };

    struct MaterialEntry
    {
        uint32_t generation = 0;
        GpuMaterial material;
    };

    struct MeshEntry
    {
        uint32_t generation = 0;
        Mesh mesh;
    };

    // Pool is declared last so it destroys descriptor sets before their
    // referenced buffers, image views, and samplers.
    DescriptorSetLayout materialDescriptorSetLayout_;
    std::vector<TextureEntry> textures_;
    std::vector<MaterialEntry> materials_;
    std::vector<MeshEntry> meshes_;
    DescriptorPool materialDescriptorPool_;
};

} // namespace VkRenderer
