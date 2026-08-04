#include "Import/GLBMaterialImporter.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{
namespace
{

TextureAssetHandle resolveTexture(
    int sourceIndex,
    const std::vector<TextureAssetHandle>* textures,
    TextureAssetHandle defaultTexture)
{
    if (sourceIndex < 0)
    {
        return defaultTexture;
    }
    if (textures == nullptr ||
        static_cast<std::size_t>(sourceIndex) >= textures->size())
    {
        throw std::invalid_argument(
            "GLB material references a texture outside the imported texture table");
    }
    return (*textures)[static_cast<std::size_t>(sourceIndex)];
}

void addTextureAssignment(
    MaterialAsset::CreateInfo& destination,
    const std::string& slotName,
    int sourceIndex,
    const GLBMaterialImporter::CreateInfo& createInfo)
{
    if (slotName.empty())
    {
        return;
    }

    const TextureAssetHandle texture = resolveTexture(
        sourceIndex,
        createInfo.textures,
        createInfo.defaultTexture);
    if (texture)
    {
        destination.textures.push_back({slotName, texture});
    }
}

} // namespace

std::vector<MaterialAssetHandle> GLBMaterialImporter::import(
    const std::vector<GLBMaterial>& source,
    const CreateInfo& createInfo) const
{
    if (createInfo.assets == nullptr)
    {
        throw std::invalid_argument(
            "GLBMaterialImporter requires an AssetManager");
    }
    if (!createInfo.assets->contains(createInfo.mapping.materialTemplate))
    {
        throw std::invalid_argument(
            "GLBMaterialImporter requires a material template owned by its AssetManager");
    }
    if (createInfo.defaultTexture &&
        !createInfo.assets->contains(createInfo.defaultTexture))
    {
        throw std::invalid_argument(
            "GLBMaterialImporter default texture is not owned by its AssetManager");
    }

    std::vector<MaterialAssetHandle> result;
    result.reserve(source.size());
    for (const GLBMaterial& material : source)
    {
        MaterialAsset::CreateInfo materialInfo{};
        materialInfo.name = material.name;
        materialInfo.materialTemplate = createInfo.mapping.materialTemplate;

        if (!createInfo.mapping.baseColorParameter.empty())
        {
            materialInfo.parameters.push_back({
                createInfo.mapping.baseColorParameter,
                material.baseColorFactor
            });
        }
        if (!createInfo.mapping.metallicParameter.empty())
        {
            materialInfo.parameters.push_back({
                createInfo.mapping.metallicParameter,
                material.metallicFactor
            });
        }
        if (!createInfo.mapping.roughnessParameter.empty())
        {
            materialInfo.parameters.push_back({
                createInfo.mapping.roughnessParameter,
                material.roughnessFactor
            });
        }
        if (!createInfo.mapping.emissiveParameter.empty())
        {
            materialInfo.parameters.push_back({
                createInfo.mapping.emissiveParameter,
                material.emissiveFactor
            });
        }

        addTextureAssignment(
            materialInfo,
            createInfo.mapping.baseColorTextureSlot,
            material.baseColorTextureIndex,
            createInfo);
        addTextureAssignment(
            materialInfo,
            createInfo.mapping.metallicRoughnessTextureSlot,
            material.metallicRoughnessTextureIndex,
            createInfo);
        addTextureAssignment(
            materialInfo,
            createInfo.mapping.normalTextureSlot,
            material.normalTextureIndex,
            createInfo);
        addTextureAssignment(
            materialInfo,
            createInfo.mapping.occlusionTextureSlot,
            material.occlusionTextureIndex,
            createInfo);
        addTextureAssignment(
            materialInfo,
            createInfo.mapping.emissiveTextureSlot,
            material.emissiveTextureIndex,
            createInfo);

        result.push_back(
            createInfo.assets->createMaterial(std::move(materialInfo)));
    }
    return result;
}

} // namespace VkRenderer
