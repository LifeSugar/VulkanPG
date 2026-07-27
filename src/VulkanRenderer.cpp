#include "VulkanRenderer.h"

#include "Mesh.h"
#include "VulkanContext.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

VulkanRenderer::VulkanRenderer(const CreateInfo& createInfo)
{
    create(createInfo);
}

VulkanRenderer::~VulkanRenderer()
{
    reset();
}

void VulkanRenderer::create(const CreateInfo& createInfo)
{
    if (createInfo.context == nullptr || !*createInfo.context)
    {
        throw std::invalid_argument(
            "cannot create VulkanRenderer with an invalid VulkanContext");
    }
    if (createInfo.framebufferExtent.width == 0 ||
        createInfo.framebufferExtent.height == 0)
    {
        throw std::invalid_argument(
            "cannot create VulkanRenderer with an empty framebuffer extent");
    }
    if (createInfo.framesInFlight == 0)
    {
        throw std::invalid_argument(
            "VulkanRenderer requires at least one frame in flight");
    }
    if (createInfo.maxRenderObjects == 0)
    {
        throw std::invalid_argument(
            "VulkanRenderer requires a non-zero render object capacity");
    }

    reset();
    context_ = createInfo.context;
    pipelineCreateInfo_ = createInfo.graphicsPipeline;

    try
    {
        const Device& device = context_->device();
        frameDataResources_.create(
            device,
            createInfo.framesInFlight,
            createInfo.maxRenderObjects);
        stagedObjectData_.reserve(createInfo.maxRenderObjects);

        SwapchainResources::CreateInfo swapchainCreateInfo{};
        swapchainCreateInfo.surface = context_->surface();
        swapchainCreateInfo.framebufferExtent =
            createInfo.framebufferExtent;
        swapchainResources_.create(device, swapchainCreateInfo);

        graphicsPipeline_.create(device, makePipelineCreateInfo());
        createFrameContexts(createInfo.framesInFlight);
    }
    catch (...)
    {
        reset();
        throw;
    }
}

void VulkanRenderer::reset() noexcept
{
    if (context_ != nullptr)
    {
        context_->waitIdle();
    }

    frameContexts_.clear();
    stagedObjectData_.clear();
    graphicsPipeline_.reset();
    swapchainResources_.reset();
    frameDataResources_.reset();
    pipelineCreateInfo_ = {};
    context_ = nullptr;
    currentFrame_ = 0;
    stagedCameraRevision_ = 0;
    hasStagedCameraData_ = false;
}

void VulkanRenderer::waitIdle() const
{
    if (context_ != nullptr)
    {
        context_->waitIdle();
    }
}

void VulkanRenderer::resize(VkExtent2D framebufferExtent)
{
    if (!*this)
    {
        throw std::logic_error("cannot resize an uninitialized VulkanRenderer");
    }
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0)
    {
        throw std::invalid_argument(
            "cannot resize VulkanRenderer to an empty framebuffer extent");
    }

    const Device& device = context_->device();
    device.waitIdle();

    // Recorded commands reference the old framebuffers and render pass.
    for (FrameContext& frame : frameContexts_)
    {
        frame.resetCommands();
    }

    SwapchainResources::CreateInfo createInfo{};
    createInfo.surface = context_->surface();
    createInfo.framebufferExtent = framebufferExtent;
    const bool pipelineCompatibilityChanged =
        swapchainResources_.recreate(device, createInfo);
    if (pipelineCompatibilityChanged)
    {
        graphicsPipeline_.create(device, makePipelineCreateInfo());
    }
}

VulkanRenderer::RenderResult VulkanRenderer::render(const RenderFrame& frameData)
{
    if (!*this)
    {
        throw std::logic_error("cannot render with an uninitialized VulkanRenderer");
    }
    if (frameData.objects.size() > frameDataResources_.objectCapacity())
    {
        throw std::invalid_argument(
            "RenderFrame exceeds VulkanRenderer object capacity");
    }
    for (const RenderObject& object : frameData.objects)
    {
        if (object.mesh == nullptr || !*object.mesh)
        {
            throw std::invalid_argument(
                "RenderFrame contains an invalid Mesh");
        }
    }

    const Device& device = context_->device();
    FrameContext& frame = frameContexts_[currentFrame_];
    frame.waitUntilReusable();

    uint32_t imageIndex = 0;
    const VkResult acquireResult = swapchainResources_.acquireNextImage(
        device,
        frame.imageAvailable(),
        imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return RenderResult::NeedsResize;
    }
    if (acquireResult != VK_SUCCESS &&
        acquireResult != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swapchain image");
    }
    bool needsResize = acquireResult == VK_SUBOPTIMAL_KHR;

    swapchainResources_.waitUntilImageReusable(device, imageIndex);
    swapchainResources_.markImageInFlight(
        imageIndex,
        frame.inFlightFence());

    updateFrameData(currentFrame_, frameData);
    frame.resetCommands();
    recordCommandBuffer(
        frame.commandBuffer(),
        imageIndex,
        frameDataResources_.descriptorSet(currentFrame_),
        frameData);

    frame.resetFence();

    const VkSemaphore waitSemaphore = frame.imageAvailable();
    const VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSemaphore signalSemaphore =
        swapchainResources_.renderFinished(imageIndex);
    const VkCommandBuffer commandBuffer = frame.commandBuffer();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &waitSemaphore;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &signalSemaphore;

    if (vkQueueSubmit(
            device.graphicsQueue(),
            1,
            &submitInfo,
            frame.inFlightFence()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    const VkResult presentResult =
        swapchainResources_.present(device.presentQueue(), imageIndex);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR)
    {
        needsResize = true;
    }
    else if (presentResult != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swapchain image");
    }

    currentFrame_ =
        (currentFrame_ + 1) % static_cast<uint32_t>(frameContexts_.size());
    return needsResize
        ? RenderResult::NeedsResize
        : RenderResult::Rendered;
}

VulkanRenderer::operator bool() const noexcept
{
    return context_ != nullptr &&
        frameDataResources_.frameCount() != 0 &&
        static_cast<bool>(swapchainResources_) &&
        static_cast<bool>(graphicsPipeline_) &&
        !frameContexts_.empty();
}

void VulkanRenderer::createFrameContexts(uint32_t frameCount)
{
    const Device& device = context_->device();
    std::vector<FrameContext> newContexts;
    newContexts.reserve(frameCount);
    for (uint32_t i = 0; i < frameCount; ++i)
    {
        newContexts.emplace_back(device);
    }
    frameContexts_ = std::move(newContexts);
    currentFrame_ = 0;
}

GraphicsPipeline::CreateInfo VulkanRenderer::makePipelineCreateInfo() const
{
    GraphicsPipeline::CreateInfo createInfo = pipelineCreateInfo_;
    createInfo.renderPass = swapchainResources_.renderPass();
    createInfo.descriptorSetLayouts = {
        frameDataResources_.descriptorSetLayout()
    };
    return createInfo;
}

void VulkanRenderer::updateFrameData(
    uint32_t frameIndex,
    const RenderFrame& frame)
{
    if (!hasStagedCameraData_ ||
        frame.view.cameraRevision != stagedCameraRevision_)
    {
        frameDataResources_.setCameraData(frame.view.cameraData);
        stagedCameraRevision_ = frame.view.cameraRevision;
        hasStagedCameraData_ = true;
    }

    stagedObjectData_.clear();
    for (const RenderObject& object : frame.objects)
    {
        stagedObjectData_.push_back(object.objectData);
    }
    if (!stagedObjectData_.empty())
    {
        frameDataResources_.setObjectData(
            stagedObjectData_.data(),
            static_cast<uint32_t>(stagedObjectData_.size()));
    }
    frameDataResources_.sync(frameIndex);
}

void VulkanRenderer::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex,
    VkDescriptorSet descriptorSet,
    const RenderFrame& frame)
{
    const VkExtent2D renderExtent = swapchainResources_.extent();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapchainResources_.renderPass();
    renderPassInfo.framebuffer = swapchainResources_.framebuffer(imageIndex);
    renderPassInfo.renderArea.extent = renderExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount =
        static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline_.get());

    VkViewport viewport{};
    viewport.width = static_cast<float>(renderExtent.width);
    viewport.height = static_cast<float>(renderExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = renderExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline_.layout(),
        0,
        1,
        &descriptorSet,
        0,
        nullptr);

    for (uint32_t objectIndex = 0;
         objectIndex < static_cast<uint32_t>(frame.objects.size());
         ++objectIndex)
    {
        const Mesh& mesh = *frame.objects[objectIndex].mesh;
        mesh.bind(commandBuffer);

        DrawPushConstants pushConstants{};
        pushConstants.objectIndex = objectIndex;
        vkCmdPushConstants(
            commandBuffer,
            graphicsPipeline_.layout(),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(pushConstants),
            &pushConstants);

        for (const SubmeshData& submesh : mesh.submeshes())
        {
            if (submesh.indexed())
            {
                vkCmdDrawIndexed(
                    commandBuffer,
                    submesh.indexCount,
                    1,
                    submesh.firstIndex,
                    0,
                    0);
            }
            else
            {
                vkCmdDraw(
                    commandBuffer,
                    submesh.vertexCount,
                    1,
                    submesh.firstVertex,
                    0);
            }
        }
    }

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer");
    }
}

} // namespace VkRenderer
