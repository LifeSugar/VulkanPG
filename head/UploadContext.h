#pragma once

#include "Buffer.h"
#include "CommandPool.h"
#include "Device.h"

namespace VkRenderer
{

class UploadContext final
{
public:
    UploadContext(const Device& device, CommandPool& commandPool);

    [[nodiscard]] Buffer uploadBuffer(
        const void* data,
        VkDeviceSize size,
        VkBufferUsageFlags destinationUsage);

private:
    void copyBuffer(
        VkBuffer source,
        VkBuffer destination,
        VkDeviceSize size);

    const Device* device_ = nullptr;
    CommandPool* commandPool_ = nullptr;
};

} // namespace VkRenderer
