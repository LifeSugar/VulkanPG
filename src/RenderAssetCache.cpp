#include "RenderAssetCache.h"

#include "Device.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

template <typename Handle>
void appendUnique(std::vector<Handle>& handles, Handle handle)
{
    if (!handle)
    {
        throw std::invalid_argument(
            "render asset graph contains an invalid handle");
    }
    if (std::find(handles.begin(), handles.end(), handle) == handles.end())
    {
        handles.push_back(handle);
    }
}

uint32_t checkedDescriptorCount(
    std::size_t materialCount,
    uint32_t texturesPerMaterial)
{
    if (texturesPerMaterial != 0 &&
        materialCount >
            std::numeric_limits<uint32_t>::max() / texturesPerMaterial)
    {
        throw std::overflow_error(
            "material texture descriptor count exceeds uint32_t");
    }
    return static_cast<uint32_t>(materialCount) * texturesPerMaterial;
}

} // namespace

RenderAssetCache::~RenderAssetCache()
{
    reset();
}

void RenderAssetCache::create(
    const Device& device,
    UploadContext& uploadContext,
    const AssetManager& assets,
    const std::vector<ModelAssetHandle>& models)
{
    if (!device || models.empty())
    {
        throw std::invalid_argument(
            "RenderAssetCache requires a device and at least one model");
    }

    std::vector<MeshAssetHandle> meshHandles;
    std::vector<MaterialAssetHandle> materialHandles;
    std::vector<TextureAssetHandle> textureHandles;
    for (ModelAssetHandle modelHandle : models)
    {
        const ModelAsset& model = assets.model(modelHandle);
        for (const ModelNode& node : model.nodes())
        {
            for (MeshAssetHandle meshHandle : node.meshes)
            {
                appendUnique(meshHandles, meshHandle);
            }
        }
    }
    for (MeshAssetHandle meshHandle : meshHandles)
    {
        const MeshAsset& meshAsset = assets.mesh(meshHandle);
        for (const SubmeshData& submesh : meshAsset.submeshes())
        {
            appendUnique(materialHandles, submesh.material);
        }
    }

    MaterialTemplateAssetHandle materialTemplate;
    uint32_t textureCount = 0;
    for (MaterialAssetHandle materialHandle : materialHandles)
    {
        const MaterialAsset& materialAsset = assets.material(materialHandle);
        if (!materialTemplate)
        {
            materialTemplate = materialAsset.materialTemplate();
            textureCount =
                static_cast<uint32_t>(materialAsset.textures().size());
        }
        else if (materialAsset.materialTemplate() != materialTemplate ||
                 materialAsset.textures().size() != textureCount)
        {
            throw std::invalid_argument(
                "one RenderAssetCache currently requires a shared material template");
        }
        for (TextureAssetHandle textureHandle : materialAsset.textures())
        {
            appendUnique(textureHandles, textureHandle);
        }
    }
    if (materialHandles.empty() || textureCount == 0)
    {
        throw std::invalid_argument(
            "render model contains no textured materials");
    }

    reset();
    try
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings(
            1 + textureCount * 2);
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        for (uint32_t index = 0; index < textureCount; ++index)
        {
            VkDescriptorSetLayoutBinding& imageBinding =
                bindings[1 + index];
            imageBinding.binding = 1 + index;
            imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageBinding.descriptorCount = 1;
            imageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding& samplerBinding =
                bindings[1 + textureCount + index];
            samplerBinding.binding = 1 + textureCount + index;
            samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerBinding.descriptorCount = 1;
            samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        materialDescriptorSetLayout_.create(device.get(), bindings);

        const uint32_t materialCount =
            static_cast<uint32_t>(materialHandles.size());
        const uint32_t materialTextureDescriptors =
            checkedDescriptorCount(materialHandles.size(), textureCount);
        materialDescriptorPool_.create(
            device.get(),
            {
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, materialCount},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, materialTextureDescriptors},
                {VK_DESCRIPTOR_TYPE_SAMPLER, materialTextureDescriptors}
            },
            materialCount);
        const std::vector<VkDescriptorSet> materialDescriptorSets =
            materialDescriptorPool_.allocate(
                materialDescriptorSetLayout_.get(),
                materialCount);

        textures_.reserve(textureHandles.size());
        for (TextureAssetHandle handle : textureHandles)
        {
            TextureEntry entry{};
            entry.handle = handle;
            entry.texture.create(
                device,
                uploadContext,
                assets.texture(handle));
            textures_.push_back(std::move(entry));
        }

        materials_.reserve(materialHandles.size());
        for (uint32_t index = 0; index < materialCount; ++index)
        {
            const MaterialAssetHandle handle = materialHandles[index];
            const MaterialAsset& materialAsset = assets.material(handle);
            std::vector<const GpuTexture*> materialTextures;
            materialTextures.reserve(textureCount);
            for (TextureAssetHandle textureHandle : materialAsset.textures())
            {
                materialTextures.push_back(&texture(textureHandle));
            }

            MaterialEntry entry{};
            entry.handle = handle;
            entry.material.create(
                device,
                materialAsset,
                materialTextures,
                materialDescriptorSets[index]);
            materials_.push_back(std::move(entry));
        }

        meshes_.reserve(meshHandles.size());
        for (MeshAssetHandle handle : meshHandles)
        {
            MeshEntry entry{};
            entry.handle = handle;
            entry.mesh.create(uploadContext, assets.mesh(handle));
            meshes_.push_back(std::move(entry));
        }
    }
    catch (...)
    {
        reset();
        throw;
    }
}

void RenderAssetCache::reset() noexcept
{
    materialDescriptorPool_.reset();
    meshes_.clear();
    materials_.clear();
    textures_.clear();
    materialDescriptorSetLayout_.reset();
}

const Mesh& RenderAssetCache::mesh(MeshAssetHandle handle) const
{
    const auto entry = std::find_if(
        meshes_.begin(),
        meshes_.end(),
        [handle](const MeshEntry& candidate)
        {
            return candidate.handle == handle;
        });
    if (entry == meshes_.end())
    {
        throw std::out_of_range("MeshAsset is absent from RenderAssetCache");
    }
    return entry->mesh;
}

const GpuMaterial& RenderAssetCache::material(
    MaterialAssetHandle handle) const
{
    const auto entry = std::find_if(
        materials_.begin(),
        materials_.end(),
        [handle](const MaterialEntry& candidate)
        {
            return candidate.handle == handle;
        });
    if (entry == materials_.end())
    {
        throw std::out_of_range(
            "MaterialAsset is absent from RenderAssetCache");
    }
    return entry->material;
}

const GpuTexture& RenderAssetCache::texture(
    TextureAssetHandle handle) const
{
    const auto entry = std::find_if(
        textures_.begin(),
        textures_.end(),
        [handle](const TextureEntry& candidate)
        {
            return candidate.handle == handle;
        });
    if (entry == textures_.end())
    {
        throw std::out_of_range(
            "TextureAsset is absent from RenderAssetCache");
    }
    return entry->texture;
}

} // namespace VkRenderer
