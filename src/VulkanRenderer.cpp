#include "VulkanRenderer.h"

#include "GpuMaterial.h"
#include "Mesh.h"
#include "VulkanContext.h"

#include <imgui_impl_vulkan.h>

#include <array>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

namespace
{

constexpr std::size_t kSceneColorAttachment = 0;
constexpr std::size_t kSceneDepthAttachment = 1;

RenderPass makeSceneRenderPass(
    const Device& device,
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // The integration step will add an explicit transition from attachment
    // writes to shader sampling between the scene and present passes.
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference{};
    colorReference.attachment =
        static_cast<uint32_t>(kSceneColorAttachment);
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference{};
    depthReference.attachment =
        static_cast<uint32_t>(kSceneDepthAttachment);
    depthReference.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pDepthStencilAttachment = &depthReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    const std::array<VkAttachmentDescription, 2> attachments = {
        colorAttachment,
        depthAttachment
    };

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount =
        static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    return RenderPass(device.get(), createInfo);
}

std::vector<RenderTarget> makeSceneRenderTargets(
    const Device& device,
    VkRenderPass renderPass,
    VkExtent2D extent,
    uint32_t frameCount,
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    RenderTarget::AttachmentInfo colorAttachment{};
    colorAttachment.format = colorFormat;
    colorAttachment.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
    colorAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    RenderTarget::AttachmentInfo depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.usage =
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthAttachment.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT |
        VK_IMAGE_ASPECT_STENCIL_BIT;

    RenderTarget::CreateInfo createInfo{};
    createInfo.renderPass = renderPass;
    createInfo.extent = extent;
    createInfo.attachments = {
        colorAttachment,
        depthAttachment
    };

    std::vector<RenderTarget> targets;
    targets.reserve(frameCount);
    for (uint32_t i = 0; i < frameCount; ++i)
    {
        targets.emplace_back(device, createInfo);
    }
    return targets;
}

bool isSrgbFormat(VkFormat format) noexcept
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return true;
    default:
        return false;
    }
}

uint32_t selectPresentOutputTransferFunction(
    VkFormat format,
    VkColorSpaceKHR colorSpace)
{
    if (isSrgbFormat(format))
    {
        // The color attachment performs linear-to-sRGB encoding.
        return 0;
    }
    if (colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
        // A UNORM attachment needs explicit encoding in the shader.
        return 1;
    }
    throw std::runtime_error(
        "present shader does not support the selected output color space");
}

VkSamplerCreateInfo makePresentSamplerCreateInfo()
{
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;
    return createInfo;
}

void updatePresentDescriptorSets(
    VkDevice device,
    const std::vector<RenderTarget>& sceneRenderTargets,
    VkSampler sampler,
    const std::vector<VkDescriptorSet>& descriptorSets)
{
    if (sceneRenderTargets.size() != descriptorSets.size())
    {
        throw std::invalid_argument(
            "present descriptor count must match scene render targets");
    }

    for (std::size_t i = 0; i < descriptorSets.size(); ++i)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageView =
            sceneRenderTargets[i].imageView(kSceneColorAttachment);
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo samplerInfo{};
        samplerInfo.sampler = sampler;

        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageInfo = &imageInfo;
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writes[1].pImageInfo = &samplerInfo;

        vkUpdateDescriptorSets(
            device,
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr);
    }
}

} // namespace

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
    presentPipelineCreateInfo_ = createInfo.presentPipeline;

    try
    {
        const Device& device = context_->device();
        frameDataResources_.create(
            device,
            createInfo.framesInFlight,
            createInfo.maxRenderObjects);

        SwapchainResources::CreateInfo swapchainCreateInfo{};
        swapchainCreateInfo.surface = context_->surface();
        swapchainCreateInfo.framebufferExtent =
            createInfo.framebufferExtent;
        swapchainResources_.create(device, swapchainCreateInfo);

        createSceneRenderTargets(
            swapchainResources_.extent(),
            createInfo.framesInFlight);
        createPresentResources(createInfo.framesInFlight);

        graphicsPipeline_.create(device, makePipelineCreateInfo());
        presentPipeline_.create(device, makePresentPipelineCreateInfo());
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
    presentPipeline_.reset();
    graphicsPipeline_.reset();
    presentDescriptorSets_.clear();
    presentDescriptorPool_.reset();
    presentDescriptorSetLayout_.reset();
    presentSampler_.reset();
    sceneRenderTargets_.clear();
    sceneRenderPass_.reset();
    swapchainResources_.reset();
    frameDataResources_.reset();
    pipelineCreateInfo_ = {};
    presentPipelineCreateInfo_ = {};
    context_ = nullptr;
    currentFrame_ = 0;
    sceneColorFormat_ = VK_FORMAT_UNDEFINED;
    sceneDepthFormat_ = VK_FORMAT_UNDEFINED;
    presentOutputTransferFunction_ = 0;
    stagedViewId_ = {};
    stagedViewGpuDataRevision_ = 0;
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

    std::vector<RenderTarget> newSceneRenderTargets =
        makeSceneRenderTargets(
            device,
            sceneRenderPass_.get(),
            swapchainResources_.extent(),
            static_cast<uint32_t>(frameContexts_.size()),
            sceneColorFormat_,
            sceneDepthFormat_);
    sceneRenderTargets_ = std::move(newSceneRenderTargets);
    recreatePresentDescriptorSets(
        static_cast<uint32_t>(frameContexts_.size()));
    presentOutputTransferFunction_ =
        selectPresentOutputTransferFunction(
            swapchainResources_.format(),
            swapchainResources_.colorSpace());

    if (pipelineCompatibilityChanged)
    {
        presentPipeline_.create(device, makePresentPipelineCreateInfo());
    }
}


//the First render() 28/7/2026
VulkanRenderer::RenderResult VulkanRenderer::render(
    const RenderFrame& frameData,
    ImDrawData* uiDrawData)
{
    if (!*this)
    {
        throw std::logic_error("cannot render with an uninitialized VulkanRenderer");
    }
    if (frameData.renderList.size() >
        frameDataResources_.objectCapacity())
    {
        throw std::invalid_argument(
            "RenderFrame exceeds VulkanRenderer object capacity");
    }
    if (!frameData.view.id || frameData.view.gpuDataRevision == 0)
    {
        throw std::invalid_argument(
            "RenderFrame contains an invalid RenderView identity or revision");
    }

    const auto validateCandidates = [](const auto& candidates)
    {
        for (const RenderItem& item : candidates)
        {
            if (item.mesh == nullptr || !*item.mesh ||
                item.material == nullptr || !*item.material ||
                item.submeshIndex >= item.mesh->submeshes().size() ||
                !item.materialKey || !item.pipelineKey)
            {
                throw std::invalid_argument(
                    "RenderFrame contains an invalid mesh, submesh, or material");
            }
        }
    };
    validateCandidates(frameData.renderList.opaque);
    validateCandidates(frameData.renderList.transparent);
    if (frameData.renderList.objectData.size() !=
        frameData.renderList.size())
    {
        throw std::invalid_argument(
            "RenderList object-data count does not match its draw count");
    }
    for (const RenderItem& item : frameData.renderList.opaque)
    {
        if (item.objectIndex >= frameData.renderList.objectData.size() ||
            isTransparentQueue(item.queue))
        {
            throw std::invalid_argument(
                "opaque RenderList contains an invalid item");
        }
    }
    for (const RenderItem& item : frameData.renderList.transparent)
    {
        if (item.objectIndex >= frameData.renderList.objectData.size() ||
            !isTransparentQueue(item.queue))
        {
            throw std::invalid_argument(
                "transparent RenderList contains an invalid item");
        }
    }

    if (!frameData.renderList.transparent.empty())
    {
        throw std::logic_error(
            "transparent RenderList requires a transparent pipeline variant");
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
        currentFrame_,
        imageIndex,
        frameDataResources_.descriptorSet(currentFrame_),
        frameData,
        uiDrawData);

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
        static_cast<bool>(sceneRenderPass_) &&
        sceneRenderTargets_.size() == frameContexts_.size() &&
        static_cast<bool>(presentSampler_) &&
        static_cast<bool>(presentDescriptorSetLayout_) &&
        static_cast<bool>(presentDescriptorPool_) &&
        presentDescriptorSets_.size() == frameContexts_.size() &&
        static_cast<bool>(graphicsPipeline_) &&
        static_cast<bool>(presentPipeline_) &&
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

void VulkanRenderer::createSceneRenderTargets(
    VkExtent2D extent,
    uint32_t frameCount)
{
    const Device& device = context_->device();
    const VkFormat colorFormat = device.findSupportedFormat(
        {VK_FORMAT_R16G16B16A16_SFLOAT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    const VkFormat depthFormat = device.findDepthStencilFormat();
    RenderPass renderPass = makeSceneRenderPass(
        device,
        colorFormat,
        depthFormat);
    std::vector<RenderTarget> targets = makeSceneRenderTargets(
        device,
        renderPass.get(),
        extent,
        frameCount,
        colorFormat,
        depthFormat);

    // Commit only after the render pass and every target succeeded.
    sceneRenderTargets_.clear();
    sceneRenderPass_.reset();
    sceneRenderPass_ = std::move(renderPass);
    sceneRenderTargets_ = std::move(targets);
    sceneColorFormat_ = colorFormat;
    sceneDepthFormat_ = depthFormat;
}

void VulkanRenderer::createPresentResources(uint32_t frameCount)
{
    const Device& device = context_->device();

    Sampler sampler(device.get(), makePresentSamplerCreateInfo());

    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    DescriptorSetLayout descriptorSetLayout(device.get(), bindings);

    const std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, frameCount},
        {VK_DESCRIPTOR_TYPE_SAMPLER, frameCount}
    };
    DescriptorPool descriptorPool(device.get(), poolSizes, frameCount);
    std::vector<VkDescriptorSet> descriptorSets =
        descriptorPool.allocate(descriptorSetLayout.get(), frameCount);
    updatePresentDescriptorSets(
        device.get(),
        sceneRenderTargets_,
        sampler.get(),
        descriptorSets);

    presentDescriptorSets_.clear();
    presentDescriptorPool_.reset();
    presentDescriptorSetLayout_.reset();
    presentSampler_.reset();
    presentSampler_ = std::move(sampler);
    presentDescriptorSetLayout_ = std::move(descriptorSetLayout);
    presentDescriptorPool_ = std::move(descriptorPool);
    presentDescriptorSets_ = std::move(descriptorSets);
    presentOutputTransferFunction_ =
        selectPresentOutputTransferFunction(
            swapchainResources_.format(),
            swapchainResources_.colorSpace());
}

void VulkanRenderer::recreatePresentDescriptorSets(uint32_t frameCount)
{
    const Device& device = context_->device();
    const std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, frameCount},
        {VK_DESCRIPTOR_TYPE_SAMPLER, frameCount}
    };
    DescriptorPool descriptorPool(device.get(), poolSizes, frameCount);
    std::vector<VkDescriptorSet> descriptorSets = descriptorPool.allocate(
        presentDescriptorSetLayout_.get(),
        frameCount);
    updatePresentDescriptorSets(
        device.get(),
        sceneRenderTargets_,
        presentSampler_.get(),
        descriptorSets);

    presentDescriptorSets_.clear();
    presentDescriptorPool_ = std::move(descriptorPool);
    presentDescriptorSets_ = std::move(descriptorSets);
}

GraphicsPipeline::CreateInfo VulkanRenderer::makePipelineCreateInfo() const
{
    GraphicsPipeline::CreateInfo createInfo = pipelineCreateInfo_;
    createInfo.renderPass = sceneRenderPass_.get();
    createInfo.descriptorSetLayouts.insert(
        createInfo.descriptorSetLayouts.begin(),
        frameDataResources_.descriptorSetLayout());
    return createInfo;
}

GraphicsPipeline::CreateInfo
VulkanRenderer::makePresentPipelineCreateInfo() const
{
    GraphicsPipeline::CreateInfo createInfo = presentPipelineCreateInfo_;
    createInfo.renderPass = swapchainResources_.renderPass();
    createInfo.descriptorSetLayouts.insert(
        createInfo.descriptorSetLayouts.begin(),
        presentDescriptorSetLayout_.get());
    return createInfo;
}

void VulkanRenderer::updateFrameData(
    uint32_t frameIndex,
    const RenderFrame& frame)
{
    if (frame.view.id != stagedViewId_ ||
        frame.view.gpuDataRevision != stagedViewGpuDataRevision_)
    {
        frameDataResources_.setCameraData(frame.view.gpuData);
        stagedViewId_ = frame.view.id;
        stagedViewGpuDataRevision_ = frame.view.gpuDataRevision;
    }

    if (!frame.renderList.objectData.empty())
    {
        frameDataResources_.setObjectData(
            frame.renderList.objectData.data(),
            static_cast<uint32_t>(frame.renderList.objectData.size()));
    }
    frameDataResources_.sync(frameIndex);
}

void VulkanRenderer::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t frameIndex,
    uint32_t imageIndex,
    VkDescriptorSet descriptorSet,
    const RenderFrame& frame,
    ImDrawData* uiDrawData)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    recordScenePass(
        commandBuffer,
        frameIndex,
        descriptorSet,
        frame);
    transitionSceneColorForSampling(commandBuffer, frameIndex);
    recordPresentPass(
        commandBuffer,
        frameIndex,
        imageIndex,
        uiDrawData);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer");
    }
}

void VulkanRenderer::recordScenePass(
    VkCommandBuffer commandBuffer,
    uint32_t frameIndex,
    VkDescriptorSet descriptorSet,
    const RenderFrame& frame)
{
    const RenderTarget& target = sceneRenderTargets_.at(frameIndex);
    const VkExtent2D renderExtent = target.extent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = sceneRenderPass_.get();
    renderPassInfo.framebuffer = target.framebuffer();
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

    const std::vector<RenderItem>& opaque = frame.renderList.opaque;
    const Mesh* boundMesh = nullptr;
    VkDescriptorSet boundMaterialDescriptorSet = VK_NULL_HANDLE;
    for (uint32_t itemIndex = 0;
         itemIndex < static_cast<uint32_t>(opaque.size());
         ++itemIndex)
    {
        const RenderItem& item = opaque[itemIndex];
        const Mesh& mesh = *item.mesh;
        if (item.mesh != boundMesh)
        {
            mesh.bind(commandBuffer);
            boundMesh = item.mesh;
        }

        const VkDescriptorSet materialDescriptorSet =
            item.material->descriptorSet();
        if (materialDescriptorSet != boundMaterialDescriptorSet)
        {
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline_.layout(),
                1,
                1,
                &materialDescriptorSet,
                0,
                nullptr);
            boundMaterialDescriptorSet = materialDescriptorSet;
        }

        DrawPushConstants pushConstants{};
        pushConstants.objectIndex = item.objectIndex;
        vkCmdPushConstants(
            commandBuffer,
            graphicsPipeline_.layout(),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(pushConstants),
            &pushConstants);

        const SubmeshData& submesh =
            mesh.submeshes()[item.submeshIndex];
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
}

void VulkanRenderer::transitionSceneColorForSampling(
    VkCommandBuffer commandBuffer,
    uint32_t frameIndex)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image =
        sceneRenderTargets_.at(frameIndex).image(kSceneColorAttachment);
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);
}

void VulkanRenderer::recordPresentPass(
    VkCommandBuffer commandBuffer,
    uint32_t frameIndex,
    uint32_t imageIndex,
    ImDrawData* uiDrawData)
{
    const VkExtent2D presentExtent = swapchainResources_.extent();

    // SwapchainResources still owns a temporary per-image depth attachment.
    // It is cleared for render-pass compatibility but the present pipeline has
    // depth testing and writes disabled. A later step removes this attachment.
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapchainResources_.renderPass();
    renderPassInfo.framebuffer =
        swapchainResources_.framebuffer(imageIndex);
    renderPassInfo.renderArea.extent = presentExtent;
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
        presentPipeline_.get());

    VkViewport viewport{};
    viewport.width = static_cast<float>(presentExtent.width);
    viewport.height = static_cast<float>(presentExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = presentExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    const VkDescriptorSet descriptorSet =
        presentDescriptorSets_.at(frameIndex);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        presentPipeline_.layout(),
        0,
        1,
        &descriptorSet,
        0,
        nullptr);

    PresentPushConstants pushConstants{};
    pushConstants.outputTransferFunction =
        presentOutputTransferFunction_;
    vkCmdPushConstants(
        commandBuffer,
        presentPipeline_.layout(),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(pushConstants),
        &pushConstants);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    if (uiDrawData != nullptr)
    {
        ImGui_ImplVulkan_RenderDrawData(uiDrawData, commandBuffer);
    }
    vkCmdEndRenderPass(commandBuffer);
}

} // namespace VkRenderer
