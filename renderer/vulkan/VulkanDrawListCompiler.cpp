#include "vulkan/VulkanDrawListCompiler.hpp"

#include "render/MaterialKey.hpp"
#include "render/RenderList.hpp"
#include "render/RenderQueue.hpp"
#include "vulkan/GpuMaterial.hpp"
#include "vulkan/Mesh.hpp"
#include "vulkan/RenderAssetCache.hpp"

#include <stdexcept>
#include <vector>

namespace rubia::rhi::vulkan
{
namespace
{

std::vector<VulkanDrawItem> compileItems(
    const std::vector<render::RenderItem>& source,
    std::size_t objectCount,
    bool transparent,
    const RenderAssetCache& resources)
{
    std::vector<VulkanDrawItem> result;
    result.reserve(source.size());
    for (const render::RenderItem& item : source)
    {
        if (!item.mesh || !item.material ||
            !item.materialKey || !item.pipelineKey ||
            item.materialKey != render::makeMaterialKey(item.material) ||
            item.objectIndex >= objectCount ||
            render::isTransparentQueue(item.queue) != transparent)
        {
            throw std::invalid_argument(
                "RenderList contains an invalid backend-neutral draw item");
        }

        const Mesh* mesh = resources.tryMesh(item.mesh);
        const GpuMaterial* material =
            resources.tryMaterial(item.material);
        if (mesh == nullptr || material == nullptr)
        {
            throw std::invalid_argument(
                "RenderList references a non-resident Vulkan resource");
        }
        if (item.submeshIndex >= mesh->submeshes().size())
        {
            throw std::out_of_range(
                "RenderList references an invalid Vulkan submesh");
        }

        const asset::MaterialRenderState& renderState = material->renderState();
        if (material->materialTemplate() !=
                item.pipelineKey.materialTemplate ||
            render::renderQueueFor(renderState) != item.queue ||
            render::makePipelineVariantKey(
                item.pipelineKey.materialTemplate,
                renderState) != item.pipelineKey)
        {
            throw std::invalid_argument(
                "RenderList material state does not match its semantic keys");
        }

        result.push_back({
            mesh,
            material,
            item.pipelineKey,
            item.submeshIndex,
            item.objectIndex
        });
    }
    return result;
}

} // namespace

VulkanDrawList VulkanDrawListCompiler::compile(
    const render::RenderList& source,
    const RenderAssetCache& resources) const
{
    VulkanDrawList result{};
    result.opaque = compileItems(
        source.opaque,
        source.objectData.size(),
        false,
        resources);
    result.transparent = compileItems(
        source.transparent,
        source.objectData.size(),
        true,
        resources);
    return result;
}

} // namespace rubia::rhi::vulkan
