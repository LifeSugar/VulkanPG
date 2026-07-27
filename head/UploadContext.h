#pragma once

#include "Buffer.h"
#include "CommandPool.h"
#include "Device.h"

namespace VkRenderer
{

/// Performs synchronous staging uploads to device-local buffers.
class UploadContext final
{
public:
    /// References a device and command pool used for synchronous uploads.
    UploadContext(const Device& device, CommandPool& commandPool);

    /// Uploads CPU data into a new device-local destination buffer.
    [[nodiscard]] Buffer uploadBuffer(
        const void* data,
        VkDeviceSize size,
        VkBufferUsageFlags destinationUsage);

private:
    /// Copies data between buffers using a one-time command submission.
    void copyBuffer(
        VkBuffer source,
        VkBuffer destination,
        VkDeviceSize size);

    /// Non-owning device used for allocation and queue submission.
    const Device* device_ = nullptr;
    /// Non-owning command pool used for transfer command buffers.
    CommandPool* commandPool_ = nullptr;
};

} // namespace VkRenderer
