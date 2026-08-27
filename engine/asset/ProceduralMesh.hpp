#pragma once

#include "asset/MeshAsset.hpp"

namespace rubia::asset
{

/// Builds a renderer-ready cube without relying on any file importer.
[[nodiscard]] MeshAsset::CreateInfo makeCubeMeshCreateInfo();

} // namespace rubia::asset
