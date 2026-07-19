#include "Image.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

namespace
{
uint32_t findMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags requiredProperties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        const bool isCompatible = (typeFilter & (1u << i)) != 0;
        const bool hasRequiredProperties =
            (memoryProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties;
        if (isCompatible && hasRequiredProperties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find a suitable image memory type!");
}
}

Image::Image(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties)
{
    create(physicalDevice, device, width, height, format, tiling, usage, memoryProperties);
}

Image::~Image()
{
    reset();
}

Image::Image(Image&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      image_(std::exchange(other.image_, VK_NULL_HANDLE)),
      memory_(std::exchange(other.memory_, VK_NULL_HANDLE))
{
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        image_ = std::exchange(other.image_, VK_NULL_HANDLE);
        memory_ = std::exchange(other.memory_, VK_NULL_HANDLE);
    }
    return *this;
}

void Image::create(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties)
{
    if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE ||
        width == 0 || height == 0 || format == VK_FORMAT_UNDEFINED)
    {
        throw std::invalid_argument("cannot create a Image with invalid arguments");
    }

    VkImage newImage = VK_NULL_HANDLE;
    VkDeviceMemory newMemory = VK_NULL_HANDLE;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &newImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    try
    {
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(device, newImage, &requirements);

        VkMemoryAllocateInfo allocationInfo{};
        allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize = requirements.size;
        allocationInfo.memoryTypeIndex =
            findMemoryType(physicalDevice, requirements.memoryTypeBits, memoryProperties);

        if (vkAllocateMemory(device, &allocationInfo, nullptr, &newMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }
        if (vkBindImageMemory(device, newImage, newMemory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to bind image memory!");
        }
    }
    catch (...)
    {
        vkDestroyImage(device, newImage, nullptr);
        if (newMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, newMemory, nullptr);
        }
        throw;
    }

    reset();
    device_ = device;
    image_ = newImage;
    memory_ = newMemory;
}

void Image::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && image_ != VK_NULL_HANDLE)
    {
        vkDestroyImage(device_, image_, nullptr);
    }
    if (device_ != VK_NULL_HANDLE && memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(device_, memory_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    image_ = VK_NULL_HANDLE;
    memory_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
