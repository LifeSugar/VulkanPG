#include "App.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

void App::createFrameContexts()
{
    std::vector<FrameContext> newContexts;
    newContexts.reserve(kMaxFramesInFlight);
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
    {
        newContexts.emplace_back(device);
    }
    frameContexts = std::move(newContexts);
    currentFrame = 0;
}

void App::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex,
    VkDescriptorSet descriptorSet)
{
    const VkExtent2D extent = swapchainResources.extent();
    const GraphicsPipeline& pipeline = graphicsPipeline;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapchainResources.renderPass();
    renderPassInfo.framebuffer = swapchainResources.framebuffer(imageIndex);
    renderPassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.get());

    VkViewport viewport{};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    mesh.bind(commandBuffer);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.layout(),
        0,
        1,
        &descriptorSet,
        0,
        nullptr);

    const DrawPushConstants pushConstants{};
    vkCmdPushConstants(
        commandBuffer,
        pipeline.layout(),
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

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void App::drawFrame()
{
    FrameContext& frame = frameContexts[currentFrame];
    frame.waitUntilReusable();

    uint32_t imageIndex = 0;
    const VkResult acquireResult = swapchainResources.acquireNextImage(
        device,
        frame.imageAvailable(),
        imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        requestSwapChainRecreation();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swapchain image!");
    }
    if (acquireResult == VK_SUBOPTIMAL_KHR)
    {
        requestSwapChainRecreation();
    }

    swapchainResources.waitUntilImageReusable(device, imageIndex);
    swapchainResources.markImageInFlight(
        imageIndex,
        frame.inFlightFence());

    updateFrameData(currentFrame);
    frame.resetCommands();
    recordCommandBuffer(
        frame.commandBuffer(),
        imageIndex,
        frameDataResources.descriptorSet(currentFrame));

    frame.resetFence();

    const VkSemaphore waitSemaphore = frame.imageAvailable();
    const VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSemaphore signalSemaphore =
        swapchainResources.renderFinished(imageIndex);
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
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    const VkResult presentResult =
        swapchainResources.present(device.presentQueue(), imageIndex);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR)
    {
        requestSwapChainRecreation();
    }
    else if (presentResult != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swapchain image!");
    }

    currentFrame =
        (currentFrame + 1) % static_cast<uint32_t>(frameContexts.size());
}

} // namespace VkRenderer
