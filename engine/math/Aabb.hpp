#pragma once

#include <glm/glm.hpp>

#include <cmath>

namespace VkRenderer
{

/// Axis-aligned bounding box represented by inclusive minimum and maximum.
struct Aabb final
{
    glm::vec3 minimum{0.0f};
    glm::vec3 maximum{0.0f};

    [[nodiscard]] glm::vec3 center() const noexcept
    {
        return (minimum + maximum) * 0.5f;
    }

    [[nodiscard]] glm::vec3 extents() const noexcept
    {
        return (maximum - minimum) * 0.5f;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return minimum.x <= maximum.x &&
            minimum.y <= maximum.y &&
            minimum.z <= maximum.z;
    }
};

/// Conservatively transforms a local AABB by an affine world transform.
[[nodiscard]] inline Aabb transformAabb(
    const Aabb& localBounds,
    const glm::mat4& worldTransform) noexcept
{
    const glm::vec3 localCenter = localBounds.center();
    const glm::vec3 localExtents = localBounds.extents();
    const glm::vec3 worldCenter = glm::vec3(
        worldTransform * glm::vec4(localCenter, 1.0f));

    const glm::mat3 linearTransform(worldTransform);
    glm::mat3 absoluteLinear(0.0f);
    for (glm::length_t column = 0; column < 3; ++column)
    {
        for (glm::length_t row = 0; row < 3; ++row)
        {
            absoluteLinear[column][row] =
                std::abs(linearTransform[column][row]);
        }
    }

    const glm::vec3 worldExtents =
        absoluteLinear * localExtents;
    return {
        worldCenter - worldExtents,
        worldCenter + worldExtents
    };
}

} // namespace VkRenderer
