#pragma once
#include "RenderData.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>


class Camera
{
public:
    struct Config
    {
        float fov = 45.0f;
        float aspectRatio = 16.0f/9.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
    };

public:
    Camera();
    ~Camera() = default;

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    void setConfig(const Config &config);
    const Config &getConfig() const;

    void setPosition(const glm::vec3 &position);
    const glm::vec3 &getPosition() const;

    void setRotation(const glm::vec3 &rotation);
    const glm::vec3 &getRotation() const;

    const glm::mat4 &getViewMatrix() const;
    const glm::mat4 &getProjectionMatrix() const;
    const glm::mat4 &getViewProjectionMatrix() const;
    [[nodiscard]] VkRenderer::CameraGpuData getGpuData() const;
    [[nodiscard]] uint64_t revision() const noexcept { return m_revision; }

    void setAspect(float aspect);

    glm::vec3 getForwardVector() const;
    glm::vec3 getRightVector() const;
    glm::vec3 getUpVector() const;

    void Update();

private:
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    Config m_config;

    mutable glm::mat4 m_viewMatrix;
    mutable glm::mat4 m_projectionMatrix;
    mutable glm::mat4 m_viewProjectionMatrix;

    mutable bool m_isViewDirty = true;
    mutable bool m_isProjectionDirty = true;
    uint64_t m_revision = 1;

private:
    void markChanged() noexcept;
    void recalculateViewMatrix() const;
    void recalculateProjectionMatrix() const;
};
