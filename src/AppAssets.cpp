#include "App.h"

#include "GLBLoader.h"
#include "Import/GLBModelImporter.h"
#include "Import/SpirvShaderImporter.h"
#include "Import/WicImageDecoder.h"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

void App::createDemoAssets()
{
    TextureAsset::CreateInfo textureInfo{};
    textureInfo.name = "Default White";
    textureInfo.width = 1;
    textureInfo.height = 1;
    textureInfo.format = TextureFormat::RGBA8UNorm;
    textureInfo.colorSpace = TextureColorSpace::Srgb;
    textureInfo.pixels = {
        std::byte{0xff},
        std::byte{0xff},
        std::byte{0xff},
        std::byte{0xff}
    };
    demoTextureAsset = assetManager.createTexture(std::move(textureInfo));

    SpirvShaderImporter shaderImporter;
    SpirvShaderImporter::CreateInfo shaderInfo{};
    shaderInfo.assets = &assetManager;
    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/triangle.vert.spv");
    shaderInfo.name = "PBR Vertex";
    shaderInfo.stage = ShaderStage::Vertex;
    pbrVertexShaderAsset = shaderImporter.import(shaderInfo);

    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/triangle.frag.spv");
    shaderInfo.name = "PBR Fragment";
    shaderInfo.stage = ShaderStage::Fragment;
    pbrFragmentShaderAsset = shaderImporter.import(shaderInfo);

    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/present.vert.spv");
    shaderInfo.name = "Present Vertex";
    shaderInfo.stage = ShaderStage::Vertex;
    presentVertexShaderAsset = shaderImporter.import(shaderInfo);

    shaderInfo.path = resolveAssetPath(
        "Assets/shaders/present.frag.spv");
    shaderInfo.name = "Present Fragment";
    shaderInfo.stage = ShaderStage::Fragment;
    presentFragmentShaderAsset = shaderImporter.import(shaderInfo);

    MaterialTemplateAsset::CreateInfo templateInfo{};
    templateInfo.name = "glTF Metallic-Roughness PBR";
    templateInfo.shaders = {
        pbrVertexShaderAsset,
        pbrFragmentShaderAsset
    };
    templateInfo.parameters = {
        {"baseColorFactor", MaterialValueType::Float4, 0, true},
        {"emissiveFactor", MaterialValueType::Float3, 16, true},
        {"metallicFactor", MaterialValueType::Float, 28, true},
        {"roughnessFactor", MaterialValueType::Float, 32, true}
    };
    templateInfo.textureSlots = {
        {"baseColorTexture", 0, true},
        {"metallicRoughnessTexture", 1, true},
        {"normalTexture", 2, true},
        {"occlusionTexture", 3, true},
        {"emissiveTexture", 4, true}
    };
    demoMaterialTemplateAsset =
        assetManager.createMaterialTemplate(std::move(templateInfo));

    MaterialAsset::CreateInfo materialInfo{};
    materialInfo.name = "Default PBR Material";
    materialInfo.materialTemplate = demoMaterialTemplateAsset;
    materialInfo.parameters = {
        {"baseColorFactor", glm::vec4(1.0f)},
        {"emissiveFactor", glm::vec3(0.0f)},
        {"metallicFactor", 0.0f},
        {"roughnessFactor", 1.0f}
    };
    materialInfo.textures = {
        {"baseColorTexture", demoTextureAsset},
        {"metallicRoughnessTexture", demoTextureAsset},
        {"normalTexture", demoTextureAsset},
        {"occlusionTexture", demoTextureAsset},
        {"emissiveTexture", demoTextureAsset}
    };
    demoMaterialAsset =
        assetManager.createMaterial(std::move(materialInfo));

    const std::string resolvedModelPath = resolveAssetPath(modelPath);
    GLBLoader loader;
    std::unique_ptr<GLBModel> sourceModel =
        loader.load(resolvedModelPath);
    if (!sourceModel)
    {
        throw std::runtime_error(
            "Failed to load model: " + modelPath + "\n" +
            loader.getLastError());
    }

    GLBModelImporter::CreateInfo importerInfo{};
    importerInfo.assets = &assetManager;
    importerInfo.baseDirectory =
        std::filesystem::path(resolvedModelPath).parent_path();
    importerInfo.defaultTexture = demoTextureAsset;
    WicImageDecoder imageDecoder;
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
    importerInfo.materialMapping.materialTemplate =
        demoMaterialTemplateAsset;
    importerInfo.materialMapping.baseColorParameter = "baseColorFactor";
    importerInfo.materialMapping.emissiveParameter = "emissiveFactor";
    importerInfo.materialMapping.metallicParameter = "metallicFactor";
    importerInfo.materialMapping.roughnessParameter = "roughnessFactor";
    importerInfo.materialMapping.baseColorTextureSlot =
        "baseColorTexture";
    importerInfo.materialMapping.metallicRoughnessTextureSlot =
        "metallicRoughnessTexture";
    importerInfo.materialMapping.normalTextureSlot = "normalTexture";
    importerInfo.materialMapping.occlusionTextureSlot =
        "occlusionTexture";
    importerInfo.materialMapping.emissiveTextureSlot =
        "emissiveTexture";
    importerInfo.fallbackMaterial = demoMaterialAsset;

    GLBModelImporter importer;
    GLBModelImporter::Result importedModel =
        importer.import(*sourceModel, importerInfo);
    if (importedModel.meshes.empty())
    {
        throw std::runtime_error(
            "Imported model contains no renderer meshes: " + modelPath);
    }
    demoModelAsset = importedModel.model;

    std::clog
        << "[Assets] Imported " << modelPath
        << ": textures=" << importedModel.textures.size()
        << ", materials=" << importedModel.materials.size()
        << ", meshes=" << importedModel.meshes.size()
        << ", nodes="
        << assetManager.model(demoModelAsset).nodes().size()
        << '\n';

    Scene::CreateInfo sceneInfo{};
    sceneInfo.name = "Demo Scene";
    SceneNode sceneRoot{};
    sceneRoot.name = "Cube Instance";
    sceneRoot.model = demoModelAsset;
    sceneInfo.nodes.push_back(std::move(sceneRoot));
    scene.create(std::move(sceneInfo));
}

} // namespace VkRenderer
