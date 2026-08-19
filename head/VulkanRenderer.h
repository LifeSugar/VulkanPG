#pragma once

#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "FrameContext.h"
#include "FrameDataResources.h"
#include "GraphicsPipeline.h"
#include "RenderFrame.h"
#include "RenderPass.h"
#include "RenderTarget.h"
#include "Sampler.h"
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
        /// Caller-supplied final presentation pipeline settings.
        GraphicsPipeline::CreateInfo presentPipeline;
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
    /// Completes final presentation pipeline settings.
    [[nodiscard]] GraphicsPipeline::CreateInfo
    makePresentPipelineCreateInfo() const;
    /// Creates one offscreen color/depth framebuffer per frame slot.
    void createSceneRenderTargets(VkExtent2D extent, uint32_t frameCount);
    /// Creates the sampler, descriptor layout, pool, and sets for presentation.
    void createPresentResources(uint32_t frameCount);
    /// Replaces descriptor sets after scene color views are recreated.
    void recreatePresentDescriptorSets(uint32_t frameCount);
    /// Stages and uploads camera and object data for one frame slot.
    void updateFrameData(uint32_t frameIndex, const RenderFrame& frame);
    /// Records all draw commands for one acquired swapchain image.
    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t frameIndex,
        uint32_t imageIndex,
        VkDescriptorSet descriptorSet,
        const RenderFrame& frame);
    /// Records all scene draws into the offscreen target for one frame slot.
    void recordScenePass(
        VkCommandBuffer commandBuffer,
        uint32_t frameIndex,
        VkDescriptorSet descriptorSet,
        const RenderFrame& frame);
    /// Makes offscreen color writes visible to the presentation shader.
    void transitionSceneColorForSampling(
        VkCommandBuffer commandBuffer,
        uint32_t frameIndex);
    /// Records the full-screen draw into one acquired swapchain image.
    void recordPresentPass(
        VkCommandBuffer commandBuffer,
        uint32_t frameIndex,
        uint32_t imageIndex);

    // Declaration order encodes destruction dependencies:
    // frames -> pipelines -> present descriptors -> scene targets
    // -> scene render pass -> swapchain -> frame-data resources.
    /// Non-owning context that must outlive the renderer.
    VulkanContext* context_ = nullptr;
    /// Pipeline settings retained for swapchain-driven rebuilds.
    GraphicsPipeline::CreateInfo pipelineCreateInfo_;
    /// Presentation pipeline settings retained for format-driven rebuilds.
    GraphicsPipeline::CreateInfo presentPipelineCreateInfo_;
    /// Per-frame descriptors and camera/object buffers.
    FrameDataResources frameDataResources_;
    /// Swapchain and all resources tied to its images.
    SwapchainResources swapchainResources_;
    /// Render pass describing the offscreen linear-color and depth outputs.
    RenderPass sceneRenderPass_;
    /// One independently reusable offscreen framebuffer per frame slot.
    std::vector<RenderTarget> sceneRenderTargets_;
    /// Linear HDR color format shared by the scene targets.
    VkFormat sceneColorFormat_ = VK_FORMAT_UNDEFINED;
    /// Depth-stencil format shared by the scene targets.
    VkFormat sceneDepthFormat_ = VK_FORMAT_UNDEFINED;
    /// Linear clamp sampler used by the presentation shader.
    Sampler presentSampler_;
    /// Descriptor interface used to sample one offscreen color image.
    DescriptorSetLayout presentDescriptorSetLayout_;
    /// Owns the per-frame presentation descriptor sets.
    DescriptorPool presentDescriptorPool_;
    /// Non-owning sets allocated from presentDescriptorPool_, indexed by F.
    std::vector<VkDescriptorSet> presentDescriptorSets_;
    /// Graphics pipeline used to record scene draws.
    GraphicsPipeline graphicsPipeline_;
    /// Full-screen pipeline used to write the acquired swapchain image.
    GraphicsPipeline presentPipeline_;
    /// Shader output transfer selected from swapchain format and color space.
    uint32_t presentOutputTransferFunction_ = 0;
    /// Reusable command and synchronization resources for in-flight frames.
    std::vector<FrameContext> frameContexts_;
    /// Frame slot selected for the next submission.
    uint32_t currentFrame_ = 0;
    /// Identity of the view whose GPU payload is currently staged.
    RenderViewId stagedViewId_{};
    /// Revision of the currently staged view GPU payload.
    uint64_t stagedViewGpuDataRevision_ = 0;
};

} // namespace VkRenderer
