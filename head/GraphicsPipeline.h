#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

class Device;

/// Owns a graphics pipeline and its pipeline layout.
class GraphicsPipeline final
{
public:
    /// Parameters used to configure a graphics pipeline.
    struct CreateInfo
    {
        /// Render pass whose subpass layout the pipeline targets.
        VkRenderPass renderPass = VK_NULL_HANDLE;
        /// CPU-owned SPIR-V imported by the Asset layer.
        std::vector<uint32_t> vertexShaderSpirv;
        std::string vertexEntryPoint = "main";
        /// CPU-owned SPIR-V imported by the Asset layer.
        std::vector<uint32_t> fragmentShaderSpirv;
        std::string fragmentEntryPoint = "main";
        /// Descriptor set layouts exposed through the pipeline layout.
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        /// Push-constant ranges exposed through the pipeline layout.
        std::vector<VkPushConstantRange> pushConstantRanges;
        /// Vertex-buffer bindings consumed by the vertex shader.
        std::vector<VkVertexInputBindingDescription> vertexBindings;
        /// Vertex attributes consumed by the vertex shader.
        std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    };

    /// Creates an empty graphics-pipeline wrapper.
    GraphicsPipeline() = default;
    /// Creates a graphics pipeline from the supplied settings.
    GraphicsPipeline(const Device& device, const CreateInfo& createInfo);
    /// Destroys the owned pipeline and pipeline layout.
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    /// Transfers pipeline ownership from another wrapper.
    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    /// Replaces this pipeline by taking ownership from another wrapper.
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    /// Creates or replaces the graphics pipeline and its layout.
    void create(const Device& device, const CreateInfo& createInfo);
    /// Destroys the pipeline and layout and clears their state.
    void reset() noexcept;

    /// Returns the owned Vulkan graphics-pipeline handle.
    [[nodiscard]] VkPipeline get() const noexcept { return pipeline_; }
    /// Returns the pipeline layout used for descriptor and push-constant binding.
    [[nodiscard]] VkPipelineLayout layout() const noexcept { return layout_; }
    /// Returns whether a graphics pipeline is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return pipeline_ != VK_NULL_HANDLE;
    }

private:
    /// Logical device that owns the pipeline resources.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan pipeline-layout handle.
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    /// Owned Vulkan graphics-pipeline handle.
    VkPipeline pipeline_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
