#include "Import/GLBModelImporter.h"

#include "GLBTypes.h"
#include "Import/GLBMeshImporter.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

uint32_t checkedSize(std::size_t size, const char* description)
{
    if (size > std::numeric_limits<uint32_t>::max())
    {
        throw std::overflow_error(
            std::string(description) + " exceeds the uint32_t range");
    }
    return static_cast<uint32_t>(size);
}

uint32_t appendNode(
    const GLBNode& source,
    uint32_t parent,
    const std::vector<MeshAssetHandle>& meshHandles,
    std::vector<ModelNode>& destination)
{
    const uint32_t nodeIndex = checkedSize(
        destination.size(),
        "imported model node index");
    destination.push_back({});

    ModelNode& node = destination[nodeIndex];
    node.name = source.name;
    node.localTransform = source.localTransform;
    node.parent = parent;
    node.meshes.reserve(source.meshIndices.size());
    for (int sourceMeshIndex : source.meshIndices)
    {
        if (sourceMeshIndex < 0 ||
            static_cast<std::size_t>(sourceMeshIndex) >= meshHandles.size())
        {
            throw std::invalid_argument(
                "GLB node references a mesh outside the document");
        }
        node.meshes.push_back(
            meshHandles[static_cast<std::size_t>(sourceMeshIndex)]);
    }

    for (const GLBNode& child : source.children)
    {
        appendNode(child, nodeIndex, meshHandles, destination);
    }
    return nodeIndex;
}

} // namespace

GLBModelImporter::Result GLBModelImporter::import(
    const GLBModel& source,
    const CreateInfo& createInfo) const
{
    if (createInfo.assets == nullptr)
    {
        throw std::invalid_argument(
            "GLBModelImporter requires an AssetManager");
    }

    Result result{};

    GLBTextureImporter::CreateInfo textureInfo{};
    textureInfo.assets = createInfo.assets;
    textureInfo.baseDirectory = createInfo.baseDirectory;
    textureInfo.colorSpace = createInfo.textureColorSpace;
    textureInfo.sampler = createInfo.textureSampler;
    textureInfo.fallbackTexture = createInfo.defaultTexture;
    textureInfo.decoder = createInfo.textureDecoder;
    result.textures = GLBTextureImporter{}.import(
        source.textures,
        textureInfo);

    if (!source.materials.empty())
    {
        GLBMaterialImporter::CreateInfo materialInfo{};
        materialInfo.assets = createInfo.assets;
        materialInfo.mapping = createInfo.materialMapping;
        materialInfo.textures = &result.textures;
        materialInfo.defaultTexture = createInfo.defaultTexture;
        result.materials = GLBMaterialImporter{}.import(
            source.materials,
            materialInfo);
    }

    GLBMeshImporter::CreateInfo meshInfo{};
    meshInfo.assets = createInfo.assets;
    meshInfo.materials = &result.materials;
    meshInfo.fallbackMaterial = createInfo.fallbackMaterial;
    result.meshes = GLBMeshImporter{}.import(source.meshes, meshInfo);

    ModelAsset::CreateInfo modelInfo{};
    modelInfo.name = source.name;
    appendNode(
        source.rootNode,
        kInvalidModelNodeIndex,
        result.meshes,
        modelInfo.nodes);
    result.model = createInfo.assets->createModel(std::move(modelInfo));
    return result;
}

} // namespace VkRenderer
