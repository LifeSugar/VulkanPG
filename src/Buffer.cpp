#include "Buffer.h"

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

    throw std::runtime_error("failed to find a suitable buffer memory type!");
}
}

Buffer::Buffer(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties)
{
    create(physicalDevice, device, size, usage, memoryProperties);
}

Buffer::~Buffer()
{
    reset();
}

Buffer::Buffer(Buffer&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      buffer_(std::exchange(other.buffer_, VK_NULL_HANDLE)),
      memory_(std::exchange(other.memory_, VK_NULL_HANDLE)),
      size_(std::exchange(other.size_, 0)),
      mappedData_(std::exchange(other.mappedData_, nullptr))
{
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        buffer_ = std::exchange(other.buffer_, VK_NULL_HANDLE);
        memory_ = std::exchange(other.memory_, VK_NULL_HANDLE);
        size_ = std::exchange(other.size_, 0);
        mappedData_ = std::exchange(other.mappedData_, nullptr);
    }
    return *this;
}

void Buffer::create(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties)
{
    if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE || size == 0)
    {
        throw std::invalid_argument("cannot create a Buffer with an invalid device or zero size");
    }

    VkBuffer newBuffer = VK_NULL_HANDLE;
    VkDeviceMemory newMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &newBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    try
    {
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, newBuffer, &requirements);

        VkMemoryAllocateInfo allocationInfo{};
        allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize = requirements.size;
        allocationInfo.memoryTypeIndex =
            findMemoryType(physicalDevice, requirements.memoryTypeBits, memoryProperties);

        if (vkAllocateMemory(device, &allocationInfo, nullptr, &newMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        if (vkBindBufferMemory(device, newBuffer, newMemory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to bind buffer memory!");
        }
    }
    catch (...)
    {
        vkDestroyBuffer(device, newBuffer, nullptr);
        if (newMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, newMemory, nullptr);
        }
        throw;
    }

    reset();
    device_ = device;
    buffer_ = newBuffer;
    memory_ = newMemory;
    size_ = size;
}

void Buffer::reset() noexcept
{
    unmap();

    if (device_ != VK_NULL_HANDLE && buffer_ != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device_, buffer_, nullptr);
    }
    if (device_ != VK_NULL_HANDLE && memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(device_, memory_, nullptr);
    }

    device_ = VK_NULL_HANDLE;
    buffer_ = VK_NULL_HANDLE;
    memory_ = VK_NULL_HANDLE;
    size_ = 0;
}

void* Buffer::map(VkDeviceSize offset, VkDeviceSize size)
{
    if (memory_ == VK_NULL_HANDLE)
    {
        throw std::logic_error("cannot map an empty Buffer");
    }
    if (mappedData_ != nullptr)
    {
        throw std::logic_error("Buffer memory is already mapped");
    }

    if (vkMapMemory(device_, memory_, offset, size, 0, &mappedData_) != VK_SUCCESS)
    {
        mappedData_ = nullptr;
        throw std::runtime_error("failed to map buffer memory!");
    }
    return mappedData_;
}

void Buffer::unmap() noexcept
{
    if (device_ != VK_NULL_HANDLE && memory_ != VK_NULL_HANDLE && mappedData_ != nullptr)
    {
        vkUnmapMemory(device_, memory_);
        mappedData_ = nullptr;
    }
}

} // namespace VkRenderer
