#include "App.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

namespace VkRenderer
{

void App::updateFrameData(uint32_t frameIndex)
{
    static const auto startTime = std::chrono::high_resolution_clock::now();

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    camera.Update();

    const uint64_t cameraRevision = camera.revision();
    if (cameraRevision != stagedCameraRevision)
    {
        frameDataResources.setCameraData(camera.getGpuData());
        stagedCameraRevision = cameraRevision;
    }

    ObjectGpuData objectData{};
    objectData.world = glm::rotate(
        glm::mat4(1.0f),
        time * glm::radians(45.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    objectData.normalMatrix = glm::transpose(glm::inverse(objectData.world));

    frameDataResources.setObjectData(objectData);
    frameDataResources.sync(frameIndex);
}

} // namespace VkRenderer
