#include "ImageView.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

ImageView::ImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags)
{
    create(device, image, format, aspectFlags);
}

ImageView::~ImageView()
{
    reset();
}

ImageView::ImageView(ImageView&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      imageView_(std::exchange(other.imageView_, VK_NULL_HANDLE))
{
}

ImageView& ImageView::operator=(ImageView&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        imageView_ = std::exchange(other.imageView_, VK_NULL_HANDLE);
    }
    return *this;
}

void ImageView::create(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags)
{
    if (device == VK_NULL_HANDLE || image == VK_NULL_HANDLE ||
        format == VK_FORMAT_UNDEFINED || aspectFlags == 0)
    {
        throw std::invalid_argument("cannot create a ImageView with invalid arguments");
    }

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView newImageView = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &createInfo, nullptr, &newImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image view!");
    }

    reset();
    device_ = device;
    imageView_ = newImageView;
}

void ImageView::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && imageView_ != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device_, imageView_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    imageView_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
