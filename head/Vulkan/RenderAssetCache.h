#pragma once

#include "Asset/AssetManager.h"
#include "Vulkan/DescriptorPool.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/GpuMaterial.h"
#include "Vulkan/GpuTexture.h"
#include "Vulkan/Mesh.h"

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
