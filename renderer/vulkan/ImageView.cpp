#include "vulkan/ImageView.hpp"

#include <stdexcept>
#include <utility>

namespace rubia::rhi::vulkan
{

ImageView::ImageView(VkDevice device, const CreateInfo& createInfo)
{
    create(device, createInfo);
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

void ImageView::create(VkDevice device, const CreateInfo& createInfo)
{
    if (device == VK_NULL_HANDLE ||
        createInfo.image == VK_NULL_HANDLE ||
        createInfo.format == VK_FORMAT_UNDEFINED ||
        createInfo.subresourceRange.aspectMask == 0 ||
        createInfo.subresourceRange.levelCount == 0 ||
        createInfo.subresourceRange.layerCount == 0)
    {
        throw std::invalid_argument(
            "cannot create an ImageView with invalid arguments");
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = createInfo.image;
    viewInfo.viewType = createInfo.type;
    viewInfo.format = createInfo.format;
    viewInfo.components = createInfo.components;
    viewInfo.subresourceRange = createInfo.subresourceRange;

    VkImageView newImageView = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &viewInfo, nullptr, &newImageView) != VK_SUCCESS)
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

} // namespace rubia::rhi::vulkan
