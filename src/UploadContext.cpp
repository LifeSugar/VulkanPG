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

Image UploadContext::uploadImage2D(
    const void* data,
    VkDeviceSize size,
    uint32_t width,
    uint32_t height,
    VkFormat format)
{
    if (data == nullptr || size == 0 || width == 0 || height == 0 ||
        format == VK_FORMAT_UNDEFINED)
    {
        throw std::invalid_argument(
            "cannot upload an image with incomplete source data");
    }
    if (size > std::numeric_limits<std::size_t>::max())
    {
        throw std::overflow_error(
            "upload image size exceeds the host address range");
    }

    Buffer stagingBuffer(
        device_->physical(),
        device_->get(),
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* mappedData = stagingBuffer.map();
    std::memcpy(mappedData, data, static_cast<std::size_t>(size));
    stagingBuffer.unmap();

    Image destinationImage(
        device_->physical(),
        device_->get(),
        width,
        height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const VkCommandBuffer commandBuffer = commandPool_->allocatePrimary();
    try
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error(
                "failed to begin image upload command buffer");
        }

        VkImageMemoryBarrier toTransfer{};
        toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = destinationImage.get();
        toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toTransfer.subresourceRange.levelCount = 1;
        toTransfer.subresourceRange.layerCount = 1;
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toTransfer);

        VkBufferImageCopy copyRegion{};
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = {width, height, 1};
        vkCmdCopyBufferToImage(
            commandBuffer,
            stagingBuffer.get(),
            destinationImage.get(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        VkImageMemoryBarrier toShaderRead = toTransfer;
        toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toShaderRead);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error(
                "failed to record image upload command buffer");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        if (vkQueueSubmit(
                device_->graphicsQueue(),
                1,
                &submitInfo,
                VK_NULL_HANDLE) != VK_SUCCESS)
        {
            throw std::runtime_error(
                "failed to submit image upload command buffer");
        }
        if (vkQueueWaitIdle(device_->graphicsQueue()) != VK_SUCCESS)
        {
            throw std::runtime_error(
                "failed to wait for image upload completion");
        }
    }
    catch (...)
    {
        commandPool_->free(commandBuffer);
        throw;
    }

    commandPool_->free(commandBuffer);
    return destinationImage;
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
