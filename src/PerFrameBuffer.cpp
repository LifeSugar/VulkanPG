#include "PerFrameBuffer.h"

#include "Device.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace VkRenderer
{

void PerFrameBuffer::create(
    const Device& device,
    uint32_t frameCount,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties)
{
    if (!device || frameCount == 0 || size == 0)
    {
        throw std::invalid_argument(
            "cannot create PerFrameBuffer with an invalid device or empty capacity");
    }

    reset();

    try
    {
        buffers_.resize(frameCount);
        for (Buffer& buffer : buffers_)
        {
            buffer.create(
                device.physical(),
                device.get(),
                size,
                usage,
                memoryProperties);
        }
        size_ = size;
        stagedData_.resize(static_cast<size_t>(size));
        uploadedVersions_.assign(frameCount, 0);
    }
    catch (...)
    {
        reset();
        throw;
    }
}

void PerFrameBuffer::reset() noexcept
{
    buffers_.clear();
    stagedData_.clear();
    uploadedVersions_.clear();
    size_ = 0;
    stagedSize_ = 0;
    version_ = 0;
    hasStagedData_ = false;
}

void PerFrameBuffer::setData(const void* data, VkDeviceSize size)
{
    if (buffers_.empty())
    {
        throw std::logic_error("cannot stage data for an empty PerFrameBuffer");
    }
    if (data == nullptr || size == 0 || size > size_)
    {
        throw std::invalid_argument("per-frame buffer data size is invalid");
    }

    std::memcpy(stagedData_.data(), data, static_cast<size_t>(size));
    stagedSize_ = size;
    hasStagedData_ = true;

    if (version_ == std::numeric_limits<uint64_t>::max())
    {
        version_ = 1;
        std::fill(uploadedVersions_.begin(), uploadedVersions_.end(), 0);
    }
    else
    {
        ++version_;
    }
}

void PerFrameBuffer::sync(uint32_t frameIndex)
{
    if (frameIndex >= buffers_.size())
    {
        throw std::out_of_range("per-frame buffer index is out of range");
    }
    if (!hasStagedData_)
    {
        throw std::logic_error("cannot sync PerFrameBuffer before staging data");
    }
    if (uploadedVersions_[frameIndex] == version_)
    {
        return;
    }

    Buffer& buffer = buffers_[frameIndex];
    void* destination = buffer.map(0, stagedSize_);
    std::memcpy(
        destination,
        stagedData_.data(),
        static_cast<size_t>(stagedSize_));
    buffer.unmap();
    uploadedVersions_[frameIndex] = version_;
}

VkBuffer PerFrameBuffer::get(uint32_t frameIndex) const
{
    if (frameIndex >= buffers_.size())
    {
        throw std::out_of_range("per-frame buffer index is out of range");
    }
    return buffers_[frameIndex].get();
}

} // namespace VkRenderer
