#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan image view.
class ImageView final
{
public:
    /// Creates an empty image-view wrapper.
    ImageView() = default;
    /// Creates a 2D view for the supplied image.
    ImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    /// Destroys the owned image view.
    ~ImageView();

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    /// Transfers image-view ownership from another wrapper.
    ImageView(ImageView&& other) noexcept;
    /// Replaces this image view by taking ownership from another wrapper.
    ImageView& operator=(ImageView&& other) noexcept;

    /// Creates or replaces the view for the supplied image.
    void create(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    /// Destroys the owned image view and clears its state.
    void reset() noexcept;

    /// Returns the owned Vulkan image-view handle.
    [[nodiscard]] VkImageView get() const noexcept { return imageView_; }
    /// Returns whether an image view is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept { return imageView_ != VK_NULL_HANDLE; }

private:
    /// Logical device that owns the image view.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan image-view handle.
    VkImageView imageView_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
