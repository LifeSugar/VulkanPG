#include "Content/DemoContent.h"

#include "Asset/AssetManager.h"
#include "GLBLoader.h"
#include "Import/GLBModelImporter.h"
#include "Import/SpirvShaderImporter.h"
#include "Import/StbImageDecoder.h"
#include "Scene/Scene.h"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace VkRenderer
{
namespace
{

constexpr const char* kDemoModelPath =
    "Assets/Models/ABeautifulGame.glb";

std::string resolveAssetPath(const std::string& relativePath)
{
    if (std::filesystem::exists(relativePath))
    {
        return relativePath;
    }

    const std::string sourcePath =
        std::string(PROJECT_SOURCE_DIR) + "/" + relativePath;
    if (std::filesystem::exists(sourcePath))
    {
        return sourcePath;
    }

    return relativePath;
}

} // namespace

DemoContent DemoContentLoader::load(
    AssetManager& assets,
    Scene& scene)
{
    DemoContent content{};

    TextureAsset::CreateInfo textureInfo{};
    textureInfo.name = "Default White";
    textureInfo.width = 1;
    textureInfo.height = 1;
    textureInfo.format = TextureFormat::RGBA8UNorm;
    textureInfo.colorSpace = TextureColorSpace::Srgb;
    textureInfo.payload = {
        std::byte{0xff},
        std::byte{0xff},
        std::byte{0xff},
        std::byte{0xff}
    };
    content.defaultTexture = assets.createTexture(std::move(textureInfo));

    SpirvShaderImporter shaderImporter;
    SpirvShaderImporter::CreateInfo shaderInfo{};
    shaderInfo.assets = &assets;
    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/triangle.vert.spv");
    shaderInfo.name = "PBR Vertex";
    shaderInfo.stage = ShaderStage::Vertex;
    content.pbrVertexShader = shaderImporter.import(shaderInfo);

    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/triangle.frag.spv");
    shaderInfo.name = "PBR Fragment";
    shaderInfo.stage = ShaderStage::Fragment;
    content.pbrFragmentShader = shaderImporter.import(shaderInfo);

    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/present.vert.spv");
    shaderInfo.name = "Present Vertex";
    shaderInfo.stage = ShaderStage::Vertex;
    content.presentVertexShader = shaderImporter.import(shaderInfo);

    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/present.frag.spv");
    shaderInfo.name = "Present Fragment";
    shaderInfo.stage = ShaderStage::Fragment;
    content.presentFragmentShader = shaderImporter.import(shaderInfo);

    MaterialTemplateAsset::CreateInfo templateInfo{};
    templateInfo.name = "glTF Metallic-Roughness PBR";
    templateInfo.shaders = {
        content.pbrVertexShader,
        content.pbrFragmentShader
    };
    templateInfo.parameters = {
        {"baseColorFactor", MaterialValueType::Float4, 0, true},
        {"emissiveFactor", MaterialValueType::Float3, 16, true},
        {"metallicFactor", MaterialValueType::Float, 28, true},
        {"roughnessFactor", MaterialValueType::Float, 32, true}
    };
    templateInfo.textureSlots = {
        {"baseColorTexture", 0, true, {1, 1}, {1, 6}},
        {"metallicRoughnessTexture", 1, true, {1, 2}, {1, 7}},
        {"normalTexture", 2, true, {1, 3}, {1, 8}},
        {"occlusionTexture", 3, true, {1, 4}, {1, 9}},
        {"emissiveTexture", 4, true, {1, 5}, {1, 10}}
    };
    content.materialTemplate =
        assets.createMaterialTemplate(std::move(templateInfo));

    MaterialAsset::CreateInfo materialInfo{};
    materialInfo.name = "Default PBR Material";
    materialInfo.materialTemplate = content.materialTemplate;
    materialInfo.parameters = {
        {"baseColorFactor", glm::vec4(1.0f)},
        {"emissiveFactor", glm::vec3(0.0f)},
        {"metallicFactor", 0.0f},
        {"roughnessFactor", 1.0f}
    };
    materialInfo.textures = {
        {"baseColorTexture", content.defaultTexture},
        {"metallicRoughnessTexture", content.defaultTexture},
        {"normalTexture", content.defaultTexture},
        {"occlusionTexture", content.defaultTexture},
        {"emissiveTexture", content.defaultTexture}
    };
    content.defaultMaterial =
        assets.createMaterial(std::move(materialInfo));

    const std::string resolvedModelPath = resolveAssetPath(kDemoModelPath);
    GLBLoader loader;
    std::unique_ptr<GLBModel> sourceModel =
        loader.load(resolvedModelPath);
    if (!sourceModel)
    {
        throw std::runtime_error(
            std::string("Failed to load model: ") + kDemoModelPath + "\n" +
            loader.getLastError());
    }

    GLBModelImporter::CreateInfo importerInfo{};
    importerInfo.assets = &assets;
    importerInfo.baseDirectory =
        std::filesystem::path(resolvedModelPath).parent_path();
    importerInfo.defaultTexture = content.defaultTexture;
    StbImageDecoder imageDecoder;
    importerInfo.textureDecoder =
        [&imageDecoder](
            const GLBTexture& texture,
            const std::filesystem::path& baseDirectory)
        {
            if (texture.storage == GLBTextureStorage::EncodedBytes)
            {
                return imageDecoder.decodeMemory(
                    texture.data,
                    texture.name);
            }
            if (texture.storage == GLBTextureStorage::ExternalUri)
            {
                return imageDecoder.decodeFile(
                    baseDirectory / texture.uri,
                    texture.name);
            }
            throw std::invalid_argument(
                "unsupported GLB texture payload passed to the image decoder");
        };
    importerInfo.materialMapping.materialTemplate = content.materialTemplate;
    importerInfo.materialMapping.baseColorParameter = "baseColorFactor";
    importerInfo.materialMapping.emissiveParameter = "emissiveFactor";
    importerInfo.materialMapping.metallicParameter = "metallicFactor";
    importerInfo.materialMapping.roughnessParameter = "roughnessFactor";
    importerInfo.materialMapping.baseColorTextureSlot = "baseColorTexture";
    importerInfo.materialMapping.metallicRoughnessTextureSlot =
        "metallicRoughnessTexture";
    importerInfo.materialMapping.normalTextureSlot = "normalTexture";
    importerInfo.materialMapping.occlusionTextureSlot = "occlusionTexture";
    importerInfo.materialMapping.emissiveTextureSlot = "emissiveTexture";
    importerInfo.fallbackMaterial = content.defaultMaterial;

    GLBModelImporter importer;
    GLBModelImporter::Result importedModel =
        importer.import(*sourceModel, importerInfo);
    if (importedModel.meshes.empty())
    {
        throw std::runtime_error(
            std::string("Imported model contains no renderer meshes: ") +
            kDemoModelPath);
    }
    content.model = importedModel.model;
    const ModelAsset& demoModel = assets.model(content.model);

    std::clog
        << "[Assets] Imported " << kDemoModelPath
        << ": textures=" << importedModel.textures.size()
        << ", materials=" << importedModel.materials.size()
        << ", meshes=" << importedModel.meshes.size()
        << ", nodes=" << demoModel.nodes().size()
        << '\n';

    Scene::CreateInfo sceneInfo{};
    sceneInfo.name = "Demo Scene";
    SceneNode sceneRoot{};
    sceneRoot.name = demoModel.name();
    sceneRoot.model = content.model;
    sceneInfo.nodes.push_back(std::move(sceneRoot));
    scene.create(std::move(sceneInfo));

    return content;
}

} // namespace VkRenderer
