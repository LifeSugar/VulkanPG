#include "UploadContext.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace VkRenderer
{

UploadContext::UploadContext(const Device& device, CommandPool& commandPool)
    : device_(&device),
      commandPool_(&commandPool)
{
    if (!device || !commandPool)
    {
        throw std::invalid_argument("cannot create an UploadContext with an invalid device or command pool");
    }
}

Buffer UploadContext::uploadBuffer(
    const void* data,
    VkDeviceSize size,
    VkBufferUsageFlags destinationUsage)
{
    if (data == nullptr || size == 0 || destinationUsage == 0)
    {
        throw std::invalid_argument("cannot upload an empty buffer or use empty destination usage flags");
    }
    if (size > std::numeric_limits<std::size_t>::max())
    {
        throw std::overflow_error("upload buffer size exceeds the host address range");
    }

    Buffer stagingBuffer(
        device_->physical(),
        device_->get(),
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mappedData = stagingBuffer.map();
    std::memcpy(mappedData, data, static_cast<std::size_t>(size));
    stagingBuffer.unmap();

    Buffer destinationBuffer(
        device_->physical(),
        device_->get(),
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | destinationUsage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copyBuffer(stagingBuffer.get(), destinationBuffer.get(), size);
    return destinationBuffer;
}

void UploadContext::copyBuffer(
    VkBuffer source,
    VkBuffer destination,
    VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = commandPool_->allocatePrimary();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        commandPool_->free(commandBuffer);
        throw std::runtime_error("failed to begin upload command buffer");
    }

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, source, destination, 1, &copyRegion);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        commandPool_->free(commandBuffer);
        throw std::runtime_error("failed to record upload command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(device_->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        commandPool_->free(commandBuffer);
        throw std::runtime_error("failed to submit upload command buffer");
    }
    if (vkQueueWaitIdle(device_->graphicsQueue()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to wait for buffer upload completion");
    }

    commandPool_->free(commandBuffer);
}

} // namespace VkRenderer
