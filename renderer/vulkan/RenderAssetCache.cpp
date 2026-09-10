#include "vulkan/RenderAssetCache.hpp"

#include "vulkan/Device.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rubia::rhi::vulkan
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
    const asset::AssetManager& assets,
    const std::vector<asset::ModelAssetHandle>& models)
{
    beginUpload(device, assets, models);
    try
    {
        while (pendingUploadCount() != 0)
        {
            uploadNext(device, uploadContext, assets);
        }
    }
    catch (...)
    {
        uploadContext.discardBatch();
        reset();
        throw;
    }
}

void RenderAssetCache::beginUpload(
    const Device& device,
    const asset::AssetManager& assets,
    const std::vector<asset::ModelAssetHandle>& models)
{
    if (!device)
    {
        throw std::invalid_argument("RenderAssetCache requires a device");
    }
    if (models.empty())
    {
        reset();
        return;
    }
    std::vector<asset::MeshAssetHandle> meshHandles;
    std::vector<asset::MaterialAssetHandle> materialHandles;
    std::vector<asset::TextureAssetHandle> textureHandles;
    for (asset::ModelAssetHandle modelHandle : models)
    {
        const asset::ModelAsset& model = assets.model(modelHandle);
        for (const asset::ModelNode& node : model.nodes())
        {
            for (asset::MeshAssetHandle meshHandle : node.meshes)
            {
                appendUnique(meshHandles, meshHandle);
            }
        }
    }
    for (asset::MeshAssetHandle meshHandle : meshHandles)
    {
        const asset::MeshAsset& meshAsset = assets.mesh(meshHandle);
        for (const asset::SubmeshData& submesh : meshAsset.submeshes())
        {
            appendUnique(materialHandles, submesh.material);
        }
    }

    asset::MaterialTemplateAssetHandle materialTemplateHandle;
    uint32_t textureCount = 0;
    for (asset::MaterialAssetHandle materialHandle : materialHandles)
    {
        const asset::MaterialAsset& materialAsset = assets.material(materialHandle);
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
        for (asset::TextureAssetHandle textureHandle : materialAsset.textures())
        {
            appendUnique(textureHandles, textureHandle);
        }
    }
    if (materialHandles.empty() || textureCount == 0)
    {
        throw std::invalid_argument(
            "render model contains no textured materials");
    }
    const asset::MaterialTemplateAsset& materialTemplate =
        assets.materialTemplate(materialTemplateHandle);
    const uint32_t textureSlotCount = static_cast<uint32_t>(
        materialTemplate.textureSlots().size());

    initialize(device, materialTemplate);
    try
    {
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
        uploadMaterialSets_ =
            materialDescriptorPool_.allocate(
                materialDescriptorSetLayout_.get(),
                materialCount);

        textures_.resize(requiredSlotCount(textureHandles));
        materials_.resize(requiredSlotCount(materialHandles));
        meshes_.resize(requiredSlotCount(meshHandles));
        pendingTextures_ = std::move(textureHandles);
        pendingMaterials_ = std::move(materialHandles);
        pendingMeshes_ = std::move(meshHandles);
    }
    catch (...)
    {
        reset();
        throw;
    }
}

void RenderAssetCache::initialize(
    const Device& device,
    const asset::MaterialTemplateAsset& materialTemplate)
{
    if (!device)
    {
        throw std::invalid_argument("RenderAssetCache requires a device");
    }
    reset();
    if (materialTemplate.parameterBlock().descriptor.set != 1)
        throw std::invalid_argument("Vulkan scene materials currently require descriptor set 1");
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& resource : materialTemplate.bindings())
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = resource.binding;
        binding.descriptorCount = resource.arrayCount;
        if (resource.stages & asset::shaderStageMask(asset::ShaderStage::Vertex))
            binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (resource.stages & asset::shaderStageMask(asset::ShaderStage::Fragment))
            binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        switch (resource.type)
        {
        case asset::ShaderResourceType::UniformBuffer:
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
        case asset::ShaderResourceType::SampledImage:
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
        case asset::ShaderResourceType::Sampler:
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; break;
        default: throw std::invalid_argument("unsupported material descriptor type");
        }
        bindings.push_back(binding);
    }
    materialDescriptorSetLayout_.create(device.get(), bindings);

}

std::size_t RenderAssetCache::pendingUploadCount() const noexcept
{
    return pendingTextures_.size() - uploadedTextures_ +
        pendingMaterials_.size() - uploadedMaterials_ +
        pendingMeshes_.size() - uploadedMeshes_;
}

void RenderAssetCache::uploadNext(
    const Device& device,
    UploadContext& uploadContext,
    const asset::AssetManager& assets)
{
    if (uploadedTextures_ < pendingTextures_.size())
    {
        const auto handle = pendingTextures_[uploadedTextures_];
        TextureEntry& entry = textures_[handle.index];
        GpuTexture::CreateInfo info{};
        info.asset = &assets.texture(handle);
        entry.texture.create(device, uploadContext, info);
        entry.generation = handle.generation;
        ++uploadedTextures_;
    }
    else if (uploadedMaterials_ < pendingMaterials_.size())
    {
        const auto handle = pendingMaterials_[uploadedMaterials_];
        const auto& materialAsset = assets.material(handle);
        const auto& materialTemplate = assets.materialTemplate(materialAsset.materialTemplate());
        std::vector<const GpuTexture*> materialTextures;
        for (auto textureHandle : materialAsset.textures())
        {
            materialTextures.push_back(&texture(textureHandle));
        }
        MaterialEntry& entry = materials_[handle.index];
        entry.material.create(device, materialAsset, materialTemplate,
            materialTextures, uploadMaterialSets_[uploadedMaterials_]);
        entry.generation = handle.generation;
        ++uploadedMaterials_;
    }
    else if (uploadedMeshes_ < pendingMeshes_.size())
    {
        const auto handle = pendingMeshes_[uploadedMeshes_];
        MeshEntry& entry = meshes_[handle.index];
        entry.mesh.create(uploadContext, assets.mesh(handle));
        entry.generation = handle.generation;
        ++uploadedMeshes_;
    }
}

GpuTexture RenderAssetCache::stageTextureReplacement(
    const Device& device,
    UploadContext& uploadContext,
    const asset::TextureAsset& replacement) const
{
    GpuTexture::CreateInfo createInfo{};
    createInfo.asset = &replacement;
    return GpuTexture(device, uploadContext, createInfo);
}

GpuTexture RenderAssetCache::commitTextureReplacement(
    const Device& device,
    const asset::AssetManager& assets,
    asset::TextureAssetHandle handle,
    GpuTexture replacement)
{
    if (!device || !replacement || tryTexture(handle) == nullptr)
    {
        throw std::invalid_argument(
            "cannot replace a texture absent from RenderAssetCache");
    }

    struct MaterialTextureUpdate
    {
        GpuMaterial* material = nullptr;
        const asset::MaterialTemplateAsset* materialTemplate = nullptr;
        std::vector<const GpuTexture*> textures;
    };
    std::vector<MaterialTextureUpdate> updates;

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(materials_.size());
         ++index)
    {
        MaterialEntry& entry = materials_[index];
        if (!entry.material || entry.generation == 0)
        {
            continue;
        }

        const asset::MaterialAssetHandle materialHandle{index, entry.generation};
        const asset::MaterialAsset& materialAsset = assets.material(materialHandle);
        if (std::find(
                materialAsset.textures().begin(),
                materialAsset.textures().end(),
                handle) == materialAsset.textures().end())
        {
            continue;
        }

        MaterialTextureUpdate update{};
        update.material = &entry.material;
        update.materialTemplate = &assets.materialTemplate(
            materialAsset.materialTemplate());
        update.textures.reserve(materialAsset.textures().size());
        for (asset::TextureAssetHandle textureHandle : materialAsset.textures())
        {
            update.textures.push_back(
                textureHandle == handle
                    ? &replacement
                    : &texture(textureHandle));
        }
        updates.push_back(std::move(update));
    }

    // All references were validated above. vkUpdateDescriptorSets has no
    // failure return; after these writes the no-throw move commits ownership.
    for (MaterialTextureUpdate& update : updates)
    {
        update.material->updateTextures(
            device,
            *update.materialTemplate,
            update.textures);
    }
    GpuTexture previous = std::move(textures_[handle.index].texture);
    textures_[handle.index].texture = std::move(replacement);
    return previous;
}

void RenderAssetCache::reset() noexcept
{
    pendingTextures_.clear();
    pendingMaterials_.clear();
    pendingMeshes_.clear();
    uploadMaterialSets_.clear();
    uploadedTextures_ = uploadedMaterials_ = uploadedMeshes_ = 0;
    materialDescriptorPool_.reset();
    meshes_.clear();
    materials_.clear();
    textures_.clear();
    materialDescriptorSetLayout_.reset();
}

const Mesh& RenderAssetCache::mesh(asset::MeshAssetHandle handle) const
{
    const Mesh* result = tryMesh(handle);
    if (result == nullptr)
    {
        throw std::out_of_range("MeshAsset is absent from RenderAssetCache");
    }
    return *result;
}

const Mesh* RenderAssetCache::tryMesh(
    asset::MeshAssetHandle handle) const noexcept
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
    asset::MaterialAssetHandle handle) const
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
    asset::MaterialAssetHandle handle) const noexcept
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
    asset::TextureAssetHandle handle) const
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
    asset::TextureAssetHandle handle) const noexcept
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

} // namespace rubia::rhi::vulkan
