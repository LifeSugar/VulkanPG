#include "Camera.h"

Camera::Camera()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_config()
    , m_viewMatrix(1.0f)
    , m_projectionMatrix(1.0f)
    , m_viewProjectionMatrix(1.0f)
    , m_isViewDirty(true)
    , m_isProjectionDirty(true)
{
}

Camera::~Camera() = default;

