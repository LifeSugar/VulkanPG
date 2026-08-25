#include "Vulkan/Image.h"

#include "Vulkan/Device.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

#if !VK_RENDERER_USE_VMA
namespace
{

uint32_t findMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags requiredProperties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
    {
        const bool isCompatible = (typeFilter & (1u << index)) != 0;
        const bool hasRequiredProperties =
            (memoryProperties.memoryTypes[index].propertyFlags &
                requiredProperties) == requiredProperties;
        if (isCompatible && hasRequiredProperties)
        {
            return index;
        }
    }

    throw std::runtime_error("failed to find a suitable image memory type!");
}

} // namespace
#endif

Image::Image(
    const Device& device,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties,
    VkSampleCountFlagBits samples)
{
    create(
        device,
        width,
        height,
        format,
        tiling,
        usage,
        memoryProperties,
        samples);
}

Image::~Image()
{
    reset();
}

Image::Image(Image&& other) noexcept
#if VK_RENDERER_USE_VMA
    : allocator_(std::exchange(other.allocator_, VK_NULL_HANDLE)),
#else
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
#endif
      image_(std::exchange(other.image_, VK_NULL_HANDLE)),
#if VK_RENDERER_USE_VMA
      allocation_(std::exchange(other.allocation_, VK_NULL_HANDLE))
#else
      memory_(std::exchange(other.memory_, VK_NULL_HANDLE))
#endif
{
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other)
    {
        reset();
#if VK_RENDERER_USE_VMA
        allocator_ = std::exchange(other.allocator_, VK_NULL_HANDLE);
#else
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
#endif
        image_ = std::exchange(other.image_, VK_NULL_HANDLE);
#if VK_RENDERER_USE_VMA
        allocation_ = std::exchange(other.allocation_, VK_NULL_HANDLE);
#else
        memory_ = std::exchange(other.memory_, VK_NULL_HANDLE);
#endif
    }
    return *this;
}

void Image::create(
    const Device& device,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties,
    VkSampleCountFlagBits samples)
{
    if (!device || width == 0 || height == 0 || format == VK_FORMAT_UNDEFINED ||
        usage == 0 || samples == 0)
    {
        throw std::invalid_argument("cannot create a Image with invalid arguments");
    }

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
    imageInfo.samples = samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#if VK_RENDERER_USE_VMA
    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationCreateInfo.requiredFlags = memoryProperties;
    if ((memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
    {
        allocationCreateInfo.flags |=
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    VkImage newImage = VK_NULL_HANDLE;
    VmaAllocation newAllocation = VK_NULL_HANDLE;
    if (vmaCreateImage(
            device.allocator(),
            &imageInfo,
            &allocationCreateInfo,
            &newImage,
            &newAllocation,
            nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create VMA image allocation!");
    }

    reset();
    allocator_ = device.allocator();
    image_ = newImage;
    allocation_ = newAllocation;
#else
    VkImage newImage = VK_NULL_HANDLE;
    VkDeviceMemory newMemory = VK_NULL_HANDLE;
    if (vkCreateImage(
            device.get(),
            &imageInfo,
            nullptr,
            &newImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    try
    {
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(
            device.get(),
            newImage,
            &requirements);

        VkMemoryAllocateInfo allocationInfo{};
        allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize = requirements.size;
        allocationInfo.memoryTypeIndex = findMemoryType(
            device.physical(),
            requirements.memoryTypeBits,
            memoryProperties);

        if (vkAllocateMemory(
                device.get(),
                &allocationInfo,
                nullptr,
                &newMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }
        if (vkBindImageMemory(
                device.get(),
                newImage,
                newMemory,
                0) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to bind image memory!");
        }
    }
    catch (...)
    {
        vkDestroyImage(device.get(), newImage, nullptr);
        if (newMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device.get(), newMemory, nullptr);
        }
        throw;
    }

    reset();
    device_ = device.get();
    image_ = newImage;
    memory_ = newMemory;
#endif
}

void Image::reset() noexcept
{
#if VK_RENDERER_USE_VMA
    if (allocator_ != VK_NULL_HANDLE && image_ != VK_NULL_HANDLE)
    {
        vmaDestroyImage(allocator_, image_, allocation_);
    }
    allocator_ = VK_NULL_HANDLE;
    image_ = VK_NULL_HANDLE;
    allocation_ = VK_NULL_HANDLE;
#else
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
#endif
}

VkDeviceMemory Image::memory() const noexcept
{
#if VK_RENDERER_USE_VMA
    if (allocator_ == VK_NULL_HANDLE || allocation_ == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }
    VmaAllocationInfo allocationInfo{};
    vmaGetAllocationInfo(allocator_, allocation_, &allocationInfo);
    return allocationInfo.deviceMemory;
#else
    return memory_;
#endif
}

} // namespace VkRenderer
