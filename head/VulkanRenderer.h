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
    /// Parameters used to create the renderer and its frame resources.
    struct CreateInfo
    {
        /// Non-owning context used for all Vulkan operations.
        VulkanContext* context = nullptr;
        /// Initial drawable size of the render target.
        VkExtent2D framebufferExtent{};
        /// Number of CPU/GPU frame slots used concurrently.
        uint32_t framesInFlight = 2;
        /// Maximum number of render objects accepted per frame.
        uint32_t maxRenderObjects = 1024;

        // renderPass and descriptorSetLayouts are supplied by VulkanRenderer.
        /// Caller-supplied graphics pipeline settings.
        GraphicsPipeline::CreateInfo graphicsPipeline;
    };

    /// Outcome of a render attempt.
    enum class RenderResult
    {
        /// The frame was submitted successfully.
        Rendered,
        /// The swapchain must be resized before rendering can continue.
        NeedsResize
    };

    /// Creates an empty renderer.
    VulkanRenderer() = default;
    /// Creates a renderer from the supplied settings.
    VulkanRenderer(const CreateInfo& createInfo);
    /// Releases all renderer-owned resources.
    ~VulkanRenderer();

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    /// Renderer ownership cannot be moved because it retains context-bound state.
    VulkanRenderer(VulkanRenderer&&) = delete;
    /// Renderer ownership cannot be move-assigned.
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    /// Creates or replaces all renderer-owned resources.
    void create(const CreateInfo& createInfo);
    /// Releases all renderer-owned resources and cached frame data.
    void reset() noexcept;
    /// Waits until all device work has completed.
    void waitIdle() const;

    /// Rebuilds resources that depend on the framebuffer size.
    void resize(VkExtent2D framebufferExtent);
    /// Records, submits, and presents one scene snapshot.
    [[nodiscard]] RenderResult render(const RenderFrame& frame);

    /// Returns the current swapchain extent.
    [[nodiscard]] VkExtent2D extent() const noexcept
    {
        return swapchainResources_.extent();
    }
    /// Returns whether the renderer is fully initialized.
    [[nodiscard]] explicit operator bool() const noexcept;

private:
    /// Creates one reusable command and synchronization context per frame slot.
    void createFrameContexts(uint32_t frameCount);
    /// Completes pipeline settings with renderer-owned layouts and render pass.
    [[nodiscard]] GraphicsPipeline::CreateInfo makePipelineCreateInfo() const;
    /// Stages and uploads camera and object data for one frame slot.
    void updateFrameData(uint32_t frameIndex, const RenderFrame& frame);
    /// Records all draw commands for one acquired swapchain image.
    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        VkDescriptorSet descriptorSet,
        const RenderFrame& frame);

    // Declaration order encodes destruction dependencies:
    // frames -> pipeline -> swapchain -> descriptor/frame buffers.
    /// Non-owning context that must outlive the renderer.
    VulkanContext* context_ = nullptr;
    /// Pipeline settings retained for swapchain-driven rebuilds.
    GraphicsPipeline::CreateInfo pipelineCreateInfo_;
    /// Per-frame descriptors and camera/object buffers.
    FrameDataResources frameDataResources_;
    /// Swapchain and all resources tied to its images.
    SwapchainResources swapchainResources_;
    /// Graphics pipeline used to record scene draws.
    GraphicsPipeline graphicsPipeline_;
    /// Reusable command and synchronization resources for in-flight frames.
    std::vector<FrameContext> frameContexts_;
    /// Contiguous CPU-side object data prepared for upload.
    std::vector<ObjectGpuData> stagedObjectData_;
    /// Frame slot selected for the next submission.
    uint32_t currentFrame_ = 0;
    /// Revision of the camera data most recently staged.
    uint64_t stagedCameraRevision_ = 0;
    /// Whether camera data has been staged at least once.
    bool hasStagedCameraData_ = false;
};

} // namespace VkRenderer
