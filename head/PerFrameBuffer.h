#pragma once

#include "Buffer.h"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace VkRenderer
{

class Device;

// Owns one buffer replica for each CPU/GPU frame slot. Replication protects
// buffer contents from being overwritten while an earlier frame still reads
// them; callers decide independently when each resource domain needs updating.
class PerFrameBuffer final
{
public:
    PerFrameBuffer() = default;
    ~PerFrameBuffer() = default;

    PerFrameBuffer(const PerFrameBuffer&) = delete;
    PerFrameBuffer& operator=(const PerFrameBuffer&) = delete;
    PerFrameBuffer(PerFrameBuffer&&) = delete;
    PerFrameBuffer& operator=(PerFrameBuffer&&) = delete;

    void create(
        const Device& device,
        uint32_t frameCount,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties);
    void reset() noexcept;

    // Replaces the CPU-side snapshot and marks every frame replica stale.
    void setData(const void* data, VkDeviceSize size);

    // Uploads the latest snapshot only when this frame replica is stale.
    void sync(uint32_t frameIndex);

    [[nodiscard]] VkBuffer get(uint32_t frameIndex) const;
    [[nodiscard]] VkDeviceSize size() const noexcept { return size_; }
    [[nodiscard]] uint32_t frameCount() const noexcept
    {
        return static_cast<uint32_t>(buffers_.size());
    }

private:
    std::vector<Buffer> buffers_;
    std::vector<std::byte> stagedData_;
    std::vector<uint64_t> uploadedVersions_;
    VkDeviceSize size_ = 0;
    uint64_t version_ = 0;
    bool hasStagedData_ = false;
};

} // namespace VkRenderer
