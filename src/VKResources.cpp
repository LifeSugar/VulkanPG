#include "App.h"
#include <stdexcept>

namespace VkRenderer
{

VkFormat App::findSupportedFormat(
    const std::vector<VkFormat> &candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device.physical(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat App::findDepthFormat() const
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

MeshData App::loadModel()
{
    model = loader.load(resolveAssetPath(modelPath));
    if (!model)
    {
        throw std::runtime_error("Failed to load model: " + modelPath + "\n" + loader.getLastError());
    }
    if (model->meshes.empty())
    {
        throw std::runtime_error("Model contains no mesh: " + modelPath);
    }

    MeshData meshData = MeshData::fromGLBMesh(model->meshes[0]);
    if (meshData.empty() || meshData.indices().empty())
    {
        throw std::runtime_error("Model mesh has no indexed geometry: " + modelPath);
    }

    return meshData;
}

} // namespace VkRenderer
