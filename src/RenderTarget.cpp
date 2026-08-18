#include "RenderTarget.h"

#include "Device.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

RenderTarget::RenderTarget(
    const Device& device,
    const CreateInfo& createInfo)
{
    create(device, createInfo);
}

RenderTarget::~RenderTarget()
{
    reset();
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : attachments_(std::move(other.attachments_)),
      framebuffer_(std::move(other.framebuffer_)),
      renderPass_(std::exchange(other.renderPass_, VK_NULL_HANDLE)),
      extent_(std::exchange(other.extent_, VkExtent2D{}))
{
}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept
{
    if (this != &other)
    {
        reset();
        attachments_ = std::move(other.attachments_);
        framebuffer_ = std::move(other.framebuffer_);
        renderPass_ = std::exchange(other.renderPass_, VK_NULL_HANDLE);
        extent_ = std::exchange(other.extent_, VkExtent2D{});
    }
    return *this;
}

void RenderTarget::create(
    const Device& device,
    const CreateInfo& createInfo)
{
    if (!device ||
        createInfo.renderPass == VK_NULL_HANDLE ||
        createInfo.extent.width == 0 ||
        createInfo.extent.height == 0 ||
        createInfo.attachments.empty())
    {
        throw std::invalid_argument("render target create info is incomplete");
    }

    std::vector<AttachmentResources> newAttachments;
    std::vector<VkImageView> newAttachmentViews;
    newAttachments.reserve(createInfo.attachments.size());
    newAttachmentViews.reserve(createInfo.attachments.size());

    for (const AttachmentInfo& info : createInfo.attachments)
    {
        if (info.format == VK_FORMAT_UNDEFINED ||
            info.usage == 0 ||
            info.aspectMask == 0 ||
            info.samples == 0)
        {
            throw std::invalid_argument(
                "render target attachment description is incomplete");
        }

        AttachmentResources resources;
        resources.info = info;
        resources.image.create(
            device.physical(),
            device.get(),
            createInfo.extent.width,
            createInfo.extent.height,
            info.format,
            info.tiling,
            info.usage,
            info.memoryProperties,
            info.samples);
        resources.view.create(
            device.get(),
            resources.image.get(),
            info.format,
            info.aspectMask);

        newAttachmentViews.push_back(resources.view.get());
        newAttachments.push_back(std::move(resources));
    }

    Framebuffer newFramebuffer(
        device.get(),
        createInfo.renderPass,
        newAttachmentViews,
        createInfo.extent);

    // Commit only after every attachment and the framebuffer succeeded.
    reset();
    attachments_ = std::move(newAttachments);
    framebuffer_ = std::move(newFramebuffer);
    renderPass_ = createInfo.renderPass;
    extent_ = createInfo.extent;
}

void RenderTarget::reset() noexcept
{
    framebuffer_.reset();
    attachments_.clear();
    renderPass_ = VK_NULL_HANDLE;
    extent_ = {};
}

VkImage RenderTarget::image(std::size_t attachmentIndex) const
{
    return attachment(attachmentIndex).image.get();
}

VkImageView RenderTarget::imageView(std::size_t attachmentIndex) const
{
    return attachment(attachmentIndex).view.get();
}

const RenderTarget::AttachmentInfo& RenderTarget::attachmentInfo(
    std::size_t attachmentIndex) const
{
    return attachment(attachmentIndex).info;
}

const RenderTarget::AttachmentResources& RenderTarget::attachment(
    std::size_t attachmentIndex) const
{
    if (attachmentIndex >= attachments_.size())
    {
        throw std::out_of_range("render target attachment index is out of range");
    }
    return attachments_[attachmentIndex];
}

} // namespace VkRenderer
