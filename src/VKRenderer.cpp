#include "App.h"
#include <array>
#include <stdexcept>

namespace VkRenderer
{

void App::createCommandPool()
{
    commandPool.create(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

void App::createCommandBuffers()
{
    const VkExtent2D extent = swapChain.extent();
    const std::vector<VkCommandBuffer> commandBuffers = commandPool.allocatePrimary(
        static_cast<uint32_t>(swapchainFrames.size()));

    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        SwapchainFrame& frame = swapchainFrames[i];
        frame.commandBuffer = commandBuffers[i];

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frame.framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline);

        mesh.bind(commandBuffers[i]);
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &frame.descriptorSet,
            0,
            nullptr
        );
        for (const SubmeshData& submesh : mesh.submeshes())
        {
            if (submesh.indexed())
            {
                vkCmdDrawIndexed(
                    commandBuffers[i],
                    submesh.indexCount,
                    1,
                    submesh.firstIndex,
                    0,
                    0);
            }
            else
            {
                vkCmdDraw(
                    commandBuffers[i],
                    submesh.vertexCount,
                    1,
                    submesh.firstVertex,
                    0);
            }
        }

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void App::createSyncObjects()
{
    framesInFlight.resize(kMaxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < kMaxFramesInFlight; i++)
    {
        FrameInFlight& frame = framesInFlight[i];
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

    updateUniformBuffer(imageIndex);

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
    submitInfo.pCommandBuffers = &swapchainFrame.commandBuffer;
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
