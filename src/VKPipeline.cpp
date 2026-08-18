#include "App.h"
#include "Asset/MeshAsset.h"

#include <array>
#include <cstddef>
#include <filesystem>

namespace VkRenderer
{

std::string App::resolveAssetPath(const std::string& relativePath)
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

GraphicsPipeline::CreateInfo App::makeGraphicsPipelineCreateInfo() const
{
    const ShaderAsset& vertexShader =
        assetManager.shader(pbrVertexShaderAsset);
    const ShaderAsset& fragmentShader =
        assetManager.shader(pbrFragmentShaderAsset);

    GraphicsPipeline::CreateInfo createInfo{};
    createInfo.vertexShaderSpirv = vertexShader.spirv();
    createInfo.vertexEntryPoint = vertexShader.entryPoint();
    createInfo.fragmentShaderSpirv = fragmentShader.spirv();
    createInfo.fragmentEntryPoint = fragmentShader.entryPoint();
    createInfo.descriptorSetLayouts = {
        renderAssets.materialDescriptorSetLayout()
    };
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = sizeof(DrawPushConstants);
    createInfo.pushConstantRanges = {pushConstantRange};

    VkVertexInputBindingDescription vertexBinding{};
    vertexBinding.binding = 0;
    vertexBinding.stride = sizeof(Vertex);
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    createInfo.vertexBindings = {vertexBinding};

    std::array<VkVertexInputAttributeDescription, 4> attributes{};
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = offsetof(Vertex, position);
    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[1].offset = offsetof(Vertex, color);
    attributes[2].binding = 0;
    attributes[2].location = 2;
    attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[2].offset = offsetof(Vertex, normal);
    attributes[3].binding = 0;
    attributes[3].location = 3;
    attributes[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[3].offset = offsetof(Vertex, texCoord);
    createInfo.vertexAttributes.assign(attributes.begin(), attributes.end());

    return createInfo;
}

GraphicsPipeline::CreateInfo App::makePresentPipelineCreateInfo() const
{
    const ShaderAsset& vertexShader =
        assetManager.shader(presentVertexShaderAsset);
    const ShaderAsset& fragmentShader =
        assetManager.shader(presentFragmentShaderAsset);

    GraphicsPipeline::CreateInfo createInfo{};
    createInfo.vertexShaderSpirv = vertexShader.spirv();
    createInfo.vertexEntryPoint = vertexShader.entryPoint();
    createInfo.fragmentShaderSpirv = fragmentShader.spirv();
    createInfo.fragmentEntryPoint = fragmentShader.entryPoint();
    createInfo.cullMode = VK_CULL_MODE_NONE;
    createInfo.depthTestEnable = VK_FALSE;
    createInfo.depthWriteEnable = VK_FALSE;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = sizeof(PresentPushConstants);
    createInfo.pushConstantRanges = {pushConstantRange};
    return createInfo;
}

} // namespace VkRenderer
