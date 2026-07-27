#pragma once

#include "Framebuffer.h"
#include "Image.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Semaphore.h"
#include "Swapchain.h"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <vector>

namespace VkRenderer
{

class Device;

// Owns resources whose lifetime follows the swapchain or one of its images.
// Window-system interaction stays outside; callers provide the framebuffer size.
class SwapchainResources final
{
public:
    struct CreateInfo
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkExtent2D framebufferExtent{};
    };

    SwapchainResources() = default;
    SwapchainResources(const Device& device, const CreateInfo& createInfo);
    ~SwapchainResources();

    SwapchainResources(const SwapchainResources&) = delete;
    SwapchainResources& operator=(const SwapchainResources&) = delete;
    SwapchainResources(SwapchainResources&& other) noexcept;
    SwapchainResources& operator=(SwapchainResources&& other) noexcept;

    void create(const Device& device, const CreateInfo& createInfo);
    // Returns true when pipelines created for the old render pass are no longer
    // compatible and must be rebuilt by the renderer/pipeline cache.
    [[nodiscard]] bool recreate(const Device& device, const CreateInfo& createInfo);
    void reset() noexcept;

    [[nodiscard]] VkResult acquireNextImage(
        const Device& device,
        VkSemaphore imageAvailable,
        uint32_t& imageIndex) const;
    [[nodiscard]] VkResult present(VkQueue presentQueue, uint32_t imageIndex) const;

    void waitUntilImageReusable(const Device& device, uint32_t imageIndex) const;
    void markImageInFlight(uint32_t imageIndex, VkFence fence);

    [[nodiscard]] VkSwapchainKHR get() const noexcept { return swapchain_.get(); }
    [[nodiscard]] VkFormat format() const noexcept { return swapchain_.format(); }
    [[nodiscard]] VkExtent2D extent() const noexcept { return swapchain_.extent(); }
    [[nodiscard]] std::size_t imageCount() const noexcept { return images_.size(); }
    [[nodiscard]] VkRenderPass renderPass() const noexcept { return renderPass_.get(); }
    [[nodiscard]] VkFramebuffer framebuffer(uint32_t imageIndex) const;
    [[nodiscard]] VkSemaphore renderFinished(uint32_t imageIndex) const;
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return static_cast<bool>(swapchain_);
    }

private:
    struct ImageResources
    {
        Image depthImage;
        ImageView depthImageView;
        Framebuffer framebuffer;
        Semaphore renderFinished;

        // Non-owning fence of the FrameContext currently using this image.
        VkFence imageInFlight = VK_NULL_HANDLE;
    };

    [[nodiscard]] static VkFormat findDepthFormat(const Device& device);
    [[nodiscard]] static RenderPass makeRenderPass(
        const Device& device,
        VkFormat colorFormat,
        VkFormat depthFormat);
    [[nodiscard]] static std::vector<ImageResources> makeImageResources(
        const Device& device,
        const Swapchain& swapchain,
        VkFormat depthFormat,
        VkRenderPass renderPass);

    [[nodiscard]] const ImageResources& image(uint32_t imageIndex) const;
    [[nodiscard]] ImageResources& image(uint32_t imageIndex);

    // Declaration order encodes destruction dependencies:
    // images -> render pass -> swapchain.
    Swapchain swapchain_;
    RenderPass renderPass_;
    std::vector<ImageResources> images_;
    VkFormat depthFormat_ = VK_FORMAT_UNDEFINED;
};

} // namespace VkRenderer
