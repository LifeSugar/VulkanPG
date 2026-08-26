#include "vulkan/DefaultPipelineFactory.hpp"

#include "asset/MeshAsset.hpp"
#include "asset/ShaderAsset.hpp"
#include "render/RenderData.hpp"

#include <array>
#include <cstddef>

namespace VkRenderer
{

GraphicsPipeline::CreateInfo makeDefaultScenePipeline(
    const ShaderAsset& vertexShader,
    const ShaderAsset& fragmentShader,
    VkDescriptorSetLayout materialDescriptorSetLayout)
{
    GraphicsPipeline::CreateInfo createInfo{};
    createInfo.vertexShaderSpirv = vertexShader.spirv();
    createInfo.vertexEntryPoint = vertexShader.entryPoint();
    createInfo.fragmentShaderSpirv = fragmentShader.spirv();
    createInfo.fragmentEntryPoint = fragmentShader.entryPoint();
    createInfo.descriptorSetLayouts = {materialDescriptorSetLayout};

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

    std::array<VkVertexInputAttributeDescription, 5> attributes{};
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
    attributes[4].binding = 0;
    attributes[4].location = 4;
    attributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[4].offset = offsetof(Vertex, tangent);
    createInfo.vertexAttributes.assign(attributes.begin(), attributes.end());

    return createInfo;
}

GraphicsPipeline::CreateInfo makeDefaultPresentPipeline(
    const ShaderAsset& vertexShader,
    const ShaderAsset& fragmentShader)
{
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
