#include "App.h"
#include <array>
#include <stdexcept>

namespace VkRenderer
{

void App::createCommandPools()
{
    framesInFlight.resize(kMaxFramesInFlight);
    for (FrameInFlight& frame : framesInFlight)
    {
        frame.commandPool.create(
            device,
            device.graphicsQueueFamily(),
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    }
}

void App::createCommandBuffers()
{
    for (FrameInFlight& frame : framesInFlight)
    {
        frame.commandBuffer = frame.commandPool.allocatePrimary();
    }
}

void App::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex,
    VkDescriptorSet descriptorSet)
{
    if (imageIndex >= swapchainFrames.size())
    {
        throw std::out_of_range("swapchain image index is out of range");
    }

    const SwapchainFrame& swapchainFrame = swapchainFrames[imageIndex];
    const VkExtent2D extent = swapChain.extent();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFrame.framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline.get());

    mesh.bind(commandBuffer);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline.layout(),
        0,
        1,
        &descriptorSet,
        0,
        nullptr
    );

    const DrawPushConstants pushConstants{};
    vkCmdPushConstants(
        commandBuffer,
        graphicsPipeline.layout(),
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

void App::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (FrameInFlight& frame : framesInFlight)
    {
        if (vkCreateSemaphore(device.get(), &semaphoreInfo, nullptr, &frame.imageAvailable) != VK_SUCCESS ||
            vkCreateFence(device.get(), &fenceInfo, nullptr, &frame.inFlight) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    createSwapchainFrameSyncObjects();
}

void App::createSwapchainFrameSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (SwapchainFrame& frame : swapchainFrames)
    {
        if (vkCreateSemaphore(device.get(), &semaphoreInfo, nullptr, &frame.renderFinished) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void App::drawFrame()
{
    FrameInFlight& frame = framesInFlight[currentFrame];
    vkWaitForFences(device.get(), 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        device.get(),
        swapChain.get(),
        UINT64_MAX,
        frame.imageAvailable,
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        requestSwapChainRecreation();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    else if (result == VK_SUBOPTIMAL_KHR)
    {
        requestSwapChainRecreation();
    }

    SwapchainFrame& swapchainFrame = swapchainFrames[imageIndex];
    if (swapchainFrame.imageInFlight != VK_NULL_HANDLE)
    {
        vkWaitForFences(
            device.get(),
            1,
            &swapchainFrame.imageInFlight,
            VK_TRUE,
            UINT64_MAX);
    }

    swapchainFrame.imageInFlight = frame.inFlight;

    updateFrameData(currentFrame);
    frame.commandPool.resetCommands();
    recordCommandBuffer(
        frame.commandBuffer,
        imageIndex,
        frameDataResources.descriptorSet(currentFrame));

    vkResetFences(device.get(), 1, &frame.inFlight);

    VkSemaphore waitSemaphores[] = { frame.imageAvailable };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { swapchainFrame.renderFinished };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, frame.inFlight) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    const VkSwapchainKHR swapchainHandle = swapChain.get();
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainHandle;
    presentInfo.pImageIndices = &imageIndex;

    VkResult resultPresent = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

    if (resultPresent == VK_ERROR_OUT_OF_DATE_KHR || resultPresent == VK_SUBOPTIMAL_KHR)
    {
        requestSwapChainRecreation();
    }
    else if (resultPresent != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
}

} // namespace VkRenderer
