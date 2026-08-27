#pragma once

#include <vulkan/vulkan.h>

namespace rubia::rhi::vulkan
{

/// RAII wrapper for a Vulkan image view.
class ImageView final
{
public:
    /// Vulkan properties of a view over an existing image.
    struct CreateInfo
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkComponentMapping components{
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY};
        VkImageSubresourceRange subresourceRange{
            0,
            0,
            1,
            0,
            1};
    };

    /// Creates an empty image-view wrapper.
    ImageView() = default;
    /// Creates a view for the supplied image.
    ImageView(VkDevice device, const CreateInfo& createInfo);
    /// Destroys the owned image view.
    ~ImageView();

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    /// Transfers image-view ownership from another wrapper.
    ImageView(ImageView&& other) noexcept;
    /// Replaces this image view by taking ownership from another wrapper.
    ImageView& operator=(ImageView&& other) noexcept;

    /// Creates or replaces the view for the supplied image.
    void create(VkDevice device, const CreateInfo& createInfo);
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

} // namespace rubia::rhi::vulkan
