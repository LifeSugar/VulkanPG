#include "VKApp.h"
#include <array>
#include <stdexcept>

void VulkanApp::createCommandPool()
{
    const QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = *queueFamilyIndices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
    
}

void VulkanApp::createCommandBuffers()
{
    const VkExtent2D extent = swapChain.extent();
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(swapChainFramebuffers.size());
    commandBuffers.resize(swapChainFramebuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
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
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline);

        VkDeviceSize offsets[] = { 0 };
        const VkBuffer vertexBufferHandle = vertexBuffer.get();
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBufferHandle, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &descriptorSets[i],
            0,
            nullptr
        );
        vkCmdDrawIndexed(commandBuffers[i], indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void VulkanApp::createSyncObjects()
{
    imageAvailableSemaphores.resize(kMaxFramesInFlight);
    renderFinishedSemaphores.resize(swapChain.imageCount());
    inFlightFences.resize(kMaxFramesInFlight);
    imagesInFlight.resize(swapChain.imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < kMaxFramesInFlight; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
           
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    for (size_t i = 0; i < swapChain.imageCount(); i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanApp::drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        device,
        swapChain.get(),
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
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

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    updateUniformBuffer(imageIndex);

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
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

    VkResult resultPresent = vkQueuePresentKHR(presentQueue, &presentInfo);

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
