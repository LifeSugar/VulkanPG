#include "Vulkan/VulkanApplicationGuiRenderBridge.h"

#include "Vulkan/GpuTexture.h"
#include "Vulkan/RenderAssetCache.h"
#include "Vulkan/VulkanRenderer.h"

#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <stdexcept>

namespace VkRenderer
{

VulkanApplicationGuiRenderBridge::~VulkanApplicationGuiRenderBridge()
{
    detach();
}

void VulkanApplicationGuiRenderBridge::attach(
    const VulkanRenderer& renderer,
    const RenderAssetCache& renderAssets)
{
    detach();
    renderer_ = &renderer;
    renderAssets_ = &renderAssets;

    if (renderer.outputMode() == VulkanRenderer::OutputMode::Editor)
    {
        registerViewportTextures();
    }
}

void VulkanApplicationGuiRenderBridge::detach() noexcept
{
    releasePreviewTextures();
    releaseViewportTextures();
    renderAssets_ = nullptr;
    renderer_ = nullptr;
}

ApplicationGuiRenderFrame
VulkanApplicationGuiRenderBridge::currentFrame()
{
    if (renderer_ == nullptr)
    {
        return {};
    }

    const VkExtent2D extent = renderer_->extent();
    ApplicationGuiRenderFrame frame{};
    frame.width = extent.width;
    frame.height = extent.height;

    if (renderer_->outputMode() != VulkanRenderer::OutputMode::Editor)
    {
        return frame;
    }

    refreshViewportTexturesIfNeeded();
    const uint32_t frameIndex = renderer_->currentFrameIndex();
    if (frameIndex < viewportTextures_.size())
    {
        frame.sceneViewport.textureId = reinterpret_cast<std::uintptr_t>(
            viewportTextures_[frameIndex]);
    }
    return frame;
}

ApplicationGuiTexture VulkanApplicationGuiRenderBridge::preview(
    TextureAssetHandle texture)
{
    if (renderAssets_ == nullptr || !texture)
    {
        return {};
    }

    const auto existing = std::find_if(
        previewTextures_.begin(),
        previewTextures_.end(),
        [texture](const TextureEntry& entry)
        {
            return entry.texture == texture;
        });
    if (existing != previewTextures_.end())
    {
        return {
            reinterpret_cast<std::uintptr_t>(existing->descriptor)
        };
    }

    const GpuTexture* gpuTexture = renderAssets_->tryTexture(texture);
    if (gpuTexture == nullptr)
    {
        return {};
    }

    const VkDescriptorSet descriptor = ImGui_ImplVulkan_AddTexture(
        gpuTexture->view(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (descriptor == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "failed to register texture preview with ImGui");
    }

    previewTextures_.push_back({texture, descriptor});
    return {reinterpret_cast<std::uintptr_t>(descriptor)};
}

void VulkanApplicationGuiRenderBridge::invalidatePreview(
    TextureAssetHandle texture) noexcept
{
    const auto firstRemoved = std::remove_if(
        previewTextures_.begin(),
        previewTextures_.end(),
        [texture](const TextureEntry& entry)
        {
            if (entry.texture != texture)
            {
                return false;
            }
            if (entry.descriptor != VK_NULL_HANDLE)
            {
                ImGui_ImplVulkan_RemoveTexture(entry.descriptor);
            }
            return true;
        });
    previewTextures_.erase(firstRemoved, previewTextures_.end());
}

void VulkanApplicationGuiRenderBridge::registerViewportTextures()
{
    if (renderer_ == nullptr ||
        renderer_->outputMode() != VulkanRenderer::OutputMode::Editor ||
        renderer_->frameCount() == 0)
    {
        return;
    }

    releaseViewportTextures();
    viewportTextures_.reserve(renderer_->frameCount());
    try
    {
        for (uint32_t frameIndex = 0;
             frameIndex < renderer_->frameCount();
             ++frameIndex)
        {
            const VulkanRenderer::EditorViewportOutput output =
                renderer_->editorViewportOutput(frameIndex);
            const VkDescriptorSet texture = ImGui_ImplVulkan_AddTexture(
                output.imageView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            if (texture == VK_NULL_HANDLE)
            {
                throw std::runtime_error(
                    "failed to register Editor viewport texture with ImGui");
            }
            viewportTextures_.push_back(texture);
            viewportTextureRevision_ = output.revision;
        }
    }
    catch (...)
    {
        releaseViewportTextures();
        throw;
    }
}

void VulkanApplicationGuiRenderBridge::refreshViewportTexturesIfNeeded()
{
    if (renderer_ == nullptr || renderer_->frameCount() == 0)
    {
        return;
    }

    const VulkanRenderer::EditorViewportOutput output =
        renderer_->editorViewportOutput(0);
    if (viewportTextures_.size() != renderer_->frameCount() ||
        viewportTextureRevision_ != output.revision)
    {
        registerViewportTextures();
    }
}

void VulkanApplicationGuiRenderBridge::releaseViewportTextures() noexcept
{
    for (VkDescriptorSet texture : viewportTextures_)
    {
        if (texture != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(texture);
        }
    }
    viewportTextures_.clear();
    viewportTextureRevision_ = 0;
}

void VulkanApplicationGuiRenderBridge::releasePreviewTextures() noexcept
{
    for (const TextureEntry& entry : previewTextures_)
    {
        if (entry.descriptor != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(entry.descriptor);
        }
    }
    previewTextures_.clear();
}

} // namespace VkRenderer
