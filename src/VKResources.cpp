#include "App.h"

#include <stdexcept>

namespace VkRenderer
{

MeshData App::loadModel()
{
    model = loader.load(resolveAssetPath(modelPath));
    if (!model)
    {
        throw std::runtime_error(
            "Failed to load model: " + modelPath + "\n" + loader.getLastError());
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
