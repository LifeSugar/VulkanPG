#pragma once

#include "Asset/MeshAsset.h"

namespace VkRenderer
{

/// Builds a renderer-ready cube without relying on any file importer.
[[nodiscard]] MeshAsset::CreateInfo makeCubeMeshCreateInfo();

} // namespace VkRenderer
