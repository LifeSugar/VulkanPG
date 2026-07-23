#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace VkRenderer
{

class Device;

class GraphicsPipeline final
{
public:
    struct CreateInfo
    {
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkExtent2D extent{};
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkPushConstantRange> pushConstantRanges;
        std::vector<VkVertexInputBindingDescription> vertexBindings;
        std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    };

    GraphicsPipeline() = default;
    GraphicsPipeline(const Device& device, const CreateInfo& createInfo);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    void create(const Device& device, const CreateInfo& createInfo);
    void reset() noexcept;

    [[nodiscard]] VkPipeline get() const noexcept { return pipeline_; }
    [[nodiscard]] VkPipelineLayout layout() const noexcept { return layout_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return pipeline_ != VK_NULL_HANDLE;
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
