#pragma once

#include "FrameContext.h"
#include "FrameDataResources.h"
#include "GraphicsPipeline.h"
#include "RenderFrame.h"
#include "SwapchainResources.h"

#include <cstdint>
#include <vector>

namespace VkRenderer
{

class Mesh;
class VulkanContext;

// Owns the Vulkan objects and synchronization needed to execute one render
// target. Scene objects and asset ownership stay outside this class.
class VulkanRenderer final
{
public:
    struct CreateInfo
    {
        VulkanContext* context = nullptr;
        VkExtent2D framebufferExtent{};
        uint32_t framesInFlight = 2;
        uint32_t maxRenderObjects = 1024;

        // renderPass and descriptorSetLayouts are supplied by VulkanRenderer.
        GraphicsPipeline::CreateInfo graphicsPipeline;
    };

    enum class RenderResult
    {
        Rendered,
        NeedsResize
    };

    VulkanRenderer() = default;
    VulkanRenderer(const CreateInfo& createInfo);
    ~VulkanRenderer();

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    void create(const CreateInfo& createInfo);
    void reset() noexcept;
    void waitIdle() const;

    void resize(VkExtent2D framebufferExtent);
    [[nodiscard]] RenderResult render(const RenderFrame& frame);

    [[nodiscard]] VkExtent2D extent() const noexcept
    {
        return swapchainResources_.extent();
    }
    [[nodiscard]] explicit operator bool() const noexcept;

private:
    void createFrameContexts(uint32_t frameCount);
    [[nodiscard]] GraphicsPipeline::CreateInfo makePipelineCreateInfo() const;
    void updateFrameData(uint32_t frameIndex, const RenderFrame& frame);
    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        VkDescriptorSet descriptorSet,
        const RenderFrame& frame);

    // Declaration order encodes destruction dependencies:
    // frames -> pipeline -> swapchain -> descriptor/frame buffers.
    VulkanContext* context_ = nullptr;
    GraphicsPipeline::CreateInfo pipelineCreateInfo_;
    FrameDataResources frameDataResources_;
    SwapchainResources swapchainResources_;
    GraphicsPipeline graphicsPipeline_;
    std::vector<FrameContext> frameContexts_;
    std::vector<ObjectGpuData> stagedObjectData_;
    uint32_t currentFrame_ = 0;
    uint64_t stagedCameraRevision_ = 0;
    bool hasStagedCameraData_ = false;
};

} // namespace VkRenderer
