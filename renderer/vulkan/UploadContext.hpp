#pragma once

#include "vulkan/Buffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"

#include <vector>

namespace VkRenderer
{

    /// Performs synchronous staging uploads to device-local buffers.
    class UploadContext final
    {
    public:
        /// Identifies an existing image and the bounds used to validate copies.
        struct ImageDestination
        {
            VkImage image = VK_NULL_HANDLE;
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkExtent3D extent{};
            uint32_t mipLevels = 0;
            uint32_t arrayLayers = 0;
        };

        /// Describes one upload into an existing image.
        struct ImageUploadInfo
        {
            const void* sourceData = nullptr;
            VkDeviceSize sourceSize = 0;
            ImageDestination destination;
            std::vector<VkBufferImageCopy> copyRegions;
            VkImageSubresourceRange destinationRange{};
            VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkPipelineStageFlags sourceStageMask =
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkAccessFlags sourceAccessMask = 0;
            VkImageLayout finalLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkPipelineStageFlags finalStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            VkAccessFlags finalAccessMask = VK_ACCESS_SHADER_READ_BIT;
        };

        /// References a device and command pool used for synchronous uploads.
        UploadContext(const Device &device, CommandPool &commandPool);

        /// Uploads CPU data into a new device-local destination buffer.
        [[nodiscard]] Buffer uploadBuffer(
            const void *data,
            VkDeviceSize size,
            VkBufferUsageFlags destinationUsage);

        /// Uploads one or more buffer regions into an existing image.
        void uploadImage(const ImageUploadInfo& uploadInfo);

    private:
        /// Copies data between buffers using a one-time command submission.
        void copyBuffer(
            VkBuffer source,
            VkBuffer destination,
            VkDeviceSize size);

        /// Non-owning device used for allocation and queue submission.
        const Device *device_ = nullptr;
        /// Non-owning command pool used for transfer command buffers.
        CommandPool *commandPool_ = nullptr;
    };

} // namespace VkRenderer
