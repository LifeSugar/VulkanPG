#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getViewProjectionMatrix() const;

    void setAspect(float aspect);

    glm::vec3 getForwardVector() const;
    glm::vec3 getRightVector() const;
    glm::vec3 getUpVector() const;

    void Update();

private:
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    Config m_config;

    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewProjectionMatrix;

    bool m_isViewDirty = true;
    bool m_isProjectionDirty = true;

private:
    void recalculateViewMatrix();
    void recalculateProjectionMatrix();
};