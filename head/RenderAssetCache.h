#pragma once

#include "Asset/AssetManager.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "GpuMaterial.h"
#include "GpuTexture.h"
#include "Mesh.h"

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
    void reset() noexcept;

    [[nodiscard]] const Mesh& mesh(MeshAssetHandle handle) const;
    [[nodiscard]] const GpuMaterial& material(
        MaterialAssetHandle handle) const;
    [[nodiscard]] VkDescriptorSetLayout materialDescriptorSetLayout() const
        noexcept
    {
        return materialDescriptorSetLayout_.get();
    }
private:
    struct TextureEntry
    {
        TextureAssetHandle handle;
        GpuTexture texture;
    };

    struct MaterialEntry
    {
        MaterialAssetHandle handle;
        GpuMaterial material;
    };

    struct MeshEntry
    {
        MeshAssetHandle handle;
        Mesh mesh;
    };

    [[nodiscard]] const GpuTexture& texture(
        TextureAssetHandle handle) const;

    // Pool is declared last so it destroys descriptor sets before their
    // referenced buffers, image views, and samplers.
    DescriptorSetLayout materialDescriptorSetLayout_;
    std::vector<TextureEntry> textures_;
    std::vector<MaterialEntry> materials_;
    std::vector<MeshEntry> meshes_;
    DescriptorPool materialDescriptorPool_;
};

} // namespace VkRenderer
