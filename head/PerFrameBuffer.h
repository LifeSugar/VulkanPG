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
    /// Creates an empty per-frame buffer set.
    PerFrameBuffer() = default;
    /// Releases all buffer replicas.
    ~PerFrameBuffer() = default;

    PerFrameBuffer(const PerFrameBuffer&) = delete;
    PerFrameBuffer& operator=(const PerFrameBuffer&) = delete;
    PerFrameBuffer(PerFrameBuffer&&) = delete;
    PerFrameBuffer& operator=(PerFrameBuffer&&) = delete;

    /// Creates one equally sized buffer for each frame slot.
    void create(
        const Device& device,
        uint32_t frameCount,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties);
    /// Releases all replicas and clears the staged snapshot.
    void reset() noexcept;

    // Replaces the used portion of the CPU-side snapshot and marks every frame
    // replica stale. The uploaded size may be smaller than the buffer capacity.
    void setData(const void* data, VkDeviceSize size);

    // Uploads the latest snapshot only when this frame replica is stale.
    void sync(uint32_t frameIndex);

    /// Returns the buffer assigned to one frame slot.
    [[nodiscard]] VkBuffer get(uint32_t frameIndex) const;
    /// Returns the capacity of each buffer replica in bytes.
    [[nodiscard]] VkDeviceSize size() const noexcept { return size_; }
    /// Returns whether a CPU-side snapshot is available for upload.
    [[nodiscard]] bool hasStagedData() const noexcept
    {
        return hasStagedData_;
    }
    /// Returns the number of allocated buffer replicas.
    [[nodiscard]] uint32_t frameCount() const noexcept
    {
        return static_cast<uint32_t>(buffers_.size());
    }

private:
    /// Buffer replica assigned to each frame slot.
    std::vector<Buffer> buffers_;
    /// Latest CPU-side data snapshot awaiting upload.
    std::vector<std::byte> stagedData_;
    /// Snapshot version currently stored in each buffer replica.
    std::vector<uint64_t> uploadedVersions_;
    /// Capacity of each buffer replica in bytes.
    VkDeviceSize size_ = 0;
    /// Used byte count of the current staged snapshot.
    VkDeviceSize stagedSize_ = 0;
    /// Monotonic version of the staged snapshot.
    uint64_t version_ = 0;
    /// Whether staged data has been provided at least once.
    bool hasStagedData_ = false;
};

} // namespace VkRenderer
