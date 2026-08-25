#include "Vulkan/UploadContext.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace VkRenderer
{
namespace
{

bool rangeFits(
    uint32_t base,
    uint32_t count,
    uint32_t available) noexcept
{
    return count != 0 && base <= available && count <= available - base;
}

uint32_t mipDimension(uint32_t base, uint32_t mipLevel) noexcept
{
    return std::max(1u, base >> std::min(mipLevel, 31u));
}

bool rangeContains(
    uint32_t outerBase,
    uint32_t outerCount,
    uint32_t innerBase,
    uint32_t innerCount) noexcept
{
    if (innerCount == 0 || innerBase < outerBase)
    {
        return false;
    }
    const uint32_t relativeBase = innerBase - outerBase;
    return relativeBase <= outerCount &&
        innerCount <= outerCount - relativeBase;
}

void validateUploadDescription(
    const UploadContext::ImageUploadInfo& uploadInfo)
{
    if (uploadInfo.sourceData == nullptr ||
        uploadInfo.sourceSize == 0)
    {
        throw std::invalid_argument(
            "image upload source data must not be empty");
    }
    if (uploadInfo.copyRegions.empty() ||
        uploadInfo.copyRegions.size() >
            std::numeric_limits<uint32_t>::max())
    {
        throw std::invalid_argument(
            "image upload must contain a valid number of copy regions");
    }
    if (uploadInfo.destination.image == VK_NULL_HANDLE ||
        uploadInfo.destination.format == VK_FORMAT_UNDEFINED ||
        uploadInfo.destination.extent.width == 0 ||
        uploadInfo.destination.extent.height == 0 ||
        uploadInfo.destination.extent.depth == 0 ||
        uploadInfo.destination.mipLevels == 0 ||
        uploadInfo.destination.arrayLayers == 0)
    {
        throw std::invalid_argument(
            "image upload destination is incomplete");
    }
    if (uploadInfo.sourceStageMask == 0 ||
        uploadInfo.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED ||
        uploadInfo.finalStageMask == 0)
    {
        throw std::invalid_argument(
            "image upload final synchronization state is invalid");
    }
}

void validateDestinationRange(
    const UploadContext::ImageUploadInfo& uploadInfo)
{
    const VkImageSubresourceRange& range = uploadInfo.destinationRange;
    if (range.aspectMask == 0 ||
        !rangeFits(
            range.baseMipLevel,
            range.levelCount,
            uploadInfo.destination.mipLevels) ||
        !rangeFits(
            range.baseArrayLayer,
            range.layerCount,
            uploadInfo.destination.arrayLayers))
    {
        throw std::invalid_argument(
            "image upload destination range is invalid");
    }
}

void validateCopySubresource(
    const VkImageSubresourceLayers& subresource,
    const VkImageSubresourceRange& destinationRange)
{
    if (subresource.aspectMask == 0 ||
        (subresource.aspectMask & destinationRange.aspectMask) !=
            subresource.aspectMask)
    {
        throw std::invalid_argument(
            "image upload copy aspect is outside the destination range");
    }
    if (!rangeContains(
            destinationRange.baseMipLevel,
            destinationRange.levelCount,
            subresource.mipLevel,
            1))
    {
        throw std::invalid_argument(
            "image upload copy mip is outside the destination range");
    }
    if (!rangeContains(
            destinationRange.baseArrayLayer,
            destinationRange.layerCount,
            subresource.baseArrayLayer,
            subresource.layerCount))
    {
        throw std::invalid_argument(
            "image upload copy layers are outside the destination range");
    }
}

void validateCopyExtent(
    const VkBufferImageCopy& region,
    const UploadContext::ImageDestination& destination)
{
    if (region.imageOffset.x < 0 ||
        region.imageOffset.y < 0 ||
        region.imageOffset.z < 0 ||
        region.imageExtent.width == 0 ||
        region.imageExtent.height == 0 ||
        region.imageExtent.depth == 0)
    {
        throw std::invalid_argument(
            "image upload copy offset or extent is invalid");
    }

    const uint32_t mipLevel = region.imageSubresource.mipLevel;
    const VkExtent3D mipExtent{
        mipDimension(destination.extent.width, mipLevel),
        mipDimension(destination.extent.height, mipLevel),
        mipDimension(destination.extent.depth, mipLevel)};
    if (static_cast<uint64_t>(region.imageOffset.x) +
            region.imageExtent.width > mipExtent.width ||
        static_cast<uint64_t>(region.imageOffset.y) +
            region.imageExtent.height > mipExtent.height ||
        static_cast<uint64_t>(region.imageOffset.z) +
            region.imageExtent.depth > mipExtent.depth)
    {
        throw std::invalid_argument(
            "image upload copy region exceeds its destination mip");
    }
}

void validateCopyRegion(
    const VkBufferImageCopy& region,
    const UploadContext::ImageUploadInfo& uploadInfo)
{
    if (region.bufferOffset >= uploadInfo.sourceSize)
    {
        throw std::invalid_argument(
            "image upload copy offset is outside the source buffer");
    }
    validateCopySubresource(
        region.imageSubresource,
        uploadInfo.destinationRange);
    validateCopyExtent(region, uploadInfo.destination);
}

void validateImageUpload(const UploadContext::ImageUploadInfo& uploadInfo)
{
    validateUploadDescription(uploadInfo);
    validateDestinationRange(uploadInfo);
    for (const VkBufferImageCopy& region : uploadInfo.copyRegions)
    {
        validateCopyRegion(region, uploadInfo);
    }
}

} // namespace

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
        *device_,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mappedData = stagingBuffer.map();
    std::memcpy(mappedData, data, static_cast<std::size_t>(size));
    stagingBuffer.unmap();

    Buffer destinationBuffer(
        *device_,
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | destinationUsage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copyBuffer(stagingBuffer.get(), destinationBuffer.get(), size);
    return destinationBuffer;
}

void UploadContext::uploadImage(const ImageUploadInfo& uploadInfo)
{
    validateImageUpload(uploadInfo);
    if (uploadInfo.sourceSize > std::numeric_limits<std::size_t>::max())
    {
        throw std::overflow_error(
            "upload image size exceeds the host address range");
    }

    Buffer stagingBuffer(
        *device_,
        uploadInfo.sourceSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* mappedData = stagingBuffer.map();
    std::memcpy(
        mappedData,
        uploadInfo.sourceData,
        static_cast<std::size_t>(uploadInfo.sourceSize));
    stagingBuffer.unmap();

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
        toTransfer.oldLayout = uploadInfo.oldLayout;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = uploadInfo.destination.image;
        toTransfer.subresourceRange = uploadInfo.destinationRange;
        toTransfer.srcAccessMask = uploadInfo.sourceAccessMask;
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(
            commandBuffer,
            uploadInfo.sourceStageMask,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toTransfer);

        vkCmdCopyBufferToImage(
            commandBuffer,
            stagingBuffer.get(),
            uploadInfo.destination.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(uploadInfo.copyRegions.size()),
            uploadInfo.copyRegions.data());

        VkImageMemoryBarrier toFinal = toTransfer;
        toFinal.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toFinal.newLayout = uploadInfo.finalLayout;
        toFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toFinal.dstAccessMask = uploadInfo.finalAccessMask;
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            uploadInfo.finalStageMask,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toFinal);

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
