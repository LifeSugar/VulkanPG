#pragma once

#include <glm/glm.hpp>

namespace VkRenderer
{

/// Draws a read-only translation/rotation/scale view of one transform.
class TransformPanel final
{
public:
    enum class Space
    {
        LocalToParent,
        World
    };

    void draw(const glm::mat4& transform, Space space) const;
};

} // namespace VkRenderer
