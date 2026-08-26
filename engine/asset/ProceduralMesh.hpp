#pragma once

#include "asset/MeshAsset.hpp"

namespace VkRenderer
{

/// Builds a renderer-ready cube without relying on any file importer.
[[nodiscard]] MeshAsset::CreateInfo makeCubeMeshCreateInfo();

} // namespace VkRenderer
