#include "Editor/Vulkan/VulkanEditorTexturePreview.h"

#include "Vulkan/GpuTexture.h"
#include "Vulkan/RenderAssetCache.h"

#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace VkRenderer
{

void VulkanEditorTexturePreview::attach(
    const RenderAssetCache& renderAssets)
{
    detach();
    renderAssets_ = &renderAssets;
}

void VulkanEditorTexturePreview::detach() noexcept
{
    for (const Entry& entry : entries_)
    {
        if (entry.descriptor != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(entry.descriptor);
        }
    }
    entries_.clear();
    renderAssets_ = nullptr;
}

EditorTexturePreview VulkanEditorTexturePreview::preview(
    TextureAssetHandle texture)
{
    if (renderAssets_ == nullptr || !texture)
    {
        return {};
    }

    const auto existing = std::find_if(
        entries_.begin(),
        entries_.end(),
        [texture](const Entry& entry)
        {
            return entry.texture == texture;
        });
    if (existing != entries_.end())
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

    entries_.push_back({texture, descriptor});
    return {reinterpret_cast<std::uintptr_t>(descriptor)};
}

} // namespace VkRenderer
