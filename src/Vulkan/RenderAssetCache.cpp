#include "Vulkan/RenderAssetCache.h"

#include "Vulkan/Device.h"

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

template <typename Handle>
std::size_t requiredSlotCount(const std::vector<Handle>& handles)
{
    std::size_t result = 0;
    for (Handle handle : handles)
    {
        result = std::max(
            result,
            static_cast<std::size_t>(handle.index) + 1);
    }
    return result;
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

    MaterialTemplateAssetHandle materialTemplateHandle;
    uint32_t textureCount = 0;
    for (MaterialAssetHandle materialHandle : materialHandles)
    {
        const MaterialAsset& materialAsset = assets.material(materialHandle);
        if (!materialTemplateHandle)
        {
            materialTemplateHandle = materialAsset.materialTemplate();
            textureCount =
                static_cast<uint32_t>(materialAsset.textures().size());
        }
        else if (materialAsset.materialTemplate() != materialTemplateHandle ||
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
    const MaterialTemplateAsset& materialTemplate =
        assets.materialTemplate(materialTemplateHandle);
    const uint32_t textureSlotCount = static_cast<uint32_t>(
        materialTemplate.textureSlots().size());

    reset();
    try
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings(
            1 + textureSlotCount * 2);
        bindings[0].binding =
            materialTemplate.parameterBlock().descriptor.binding;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        for (uint32_t index = 0; index < textureSlotCount; ++index)
        {
            const MaterialTextureSlotDesc& slot =
                materialTemplate.textureSlots()[index];
            VkDescriptorSetLayoutBinding& imageBinding =
                bindings[1 + index];
            imageBinding.binding = slot.imageBinding.binding;
            imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageBinding.descriptorCount = 1;
            imageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding& samplerBinding =
                bindings[1 + textureSlotCount + index];
            samplerBinding.binding = slot.samplerBinding.binding;
            samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerBinding.descriptorCount = 1;
            samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        materialDescriptorSetLayout_.create(device.get(), bindings);

        const uint32_t materialCount =
            static_cast<uint32_t>(materialHandles.size());
        const uint32_t materialTextureDescriptors =
            checkedDescriptorCount(
                materialHandles.size(),
                textureSlotCount);
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

        textures_.resize(requiredSlotCount(textureHandles));
        for (TextureAssetHandle handle : textureHandles)
        {
            TextureEntry& entry = textures_[handle.index];
            entry.generation = handle.generation;
            entry.texture.create(
                device,
                uploadContext,
                assets.texture(handle));
        }

        materials_.resize(requiredSlotCount(materialHandles));
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

            MaterialEntry& entry = materials_[handle.index];
            entry.generation = handle.generation;
            entry.material.create(
                device,
                materialAsset,
                materialTemplate,
                materialTextures,
                materialDescriptorSets[index]);
        }

        meshes_.resize(requiredSlotCount(meshHandles));
        for (MeshAssetHandle handle : meshHandles)
        {
            MeshEntry& entry = meshes_[handle.index];
            entry.generation = handle.generation;
            entry.mesh.create(uploadContext, assets.mesh(handle));
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
    const Mesh* result = tryMesh(handle);
    if (result == nullptr)
    {
        throw std::out_of_range("MeshAsset is absent from RenderAssetCache");
    }
    return *result;
}

const Mesh* RenderAssetCache::tryMesh(
    MeshAssetHandle handle) const noexcept
{
    if (!handle || handle.index >= meshes_.size())
    {
        return nullptr;
    }
    const MeshEntry& entry = meshes_[handle.index];
    return entry.generation == handle.generation && entry.mesh
        ? &entry.mesh
        : nullptr;
}

const GpuMaterial& RenderAssetCache::material(
    MaterialAssetHandle handle) const
{
    const GpuMaterial* result = tryMaterial(handle);
    if (result == nullptr)
    {
        throw std::out_of_range(
            "MaterialAsset is absent from RenderAssetCache");
    }
    return *result;
}

const GpuMaterial* RenderAssetCache::tryMaterial(
    MaterialAssetHandle handle) const noexcept
{
    if (!handle || handle.index >= materials_.size())
    {
        return nullptr;
    }
    const MaterialEntry& entry = materials_[handle.index];
    return entry.generation == handle.generation && entry.material
        ? &entry.material
        : nullptr;
}

const GpuTexture& RenderAssetCache::texture(
    TextureAssetHandle handle) const
{
    const GpuTexture* result = tryTexture(handle);
    if (result == nullptr)
    {
        throw std::out_of_range(
            "TextureAsset is absent from RenderAssetCache");
    }
    return *result;
}

const GpuTexture* RenderAssetCache::tryTexture(
    TextureAssetHandle handle) const noexcept
{
    if (!handle || handle.index >= textures_.size())
    {
        return nullptr;
    }
    const TextureEntry& entry = textures_[handle.index];
    return entry.generation == handle.generation && entry.texture
        ? &entry.texture
        : nullptr;
}

} // namespace VkRenderer
