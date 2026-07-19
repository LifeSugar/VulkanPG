#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

class ImageView final
{
public:
    ImageView() = default;
    ImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    ~ImageView();

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    ImageView(ImageView&& other) noexcept;
    ImageView& operator=(ImageView&& other) noexcept;

    void create(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void reset() noexcept;

    [[nodiscard]] VkImageView get() const noexcept { return imageView_; }
    [[nodiscard]] explicit operator bool() const noexcept { return imageView_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkImageView imageView_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
