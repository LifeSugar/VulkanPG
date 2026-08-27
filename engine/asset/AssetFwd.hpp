#pragma once

#include "asset/AssetHandle.hpp"

namespace rubia::asset
{

class MaterialAsset;
class MaterialTemplateAsset;
class MeshAsset;
class ModelAsset;
class ShaderAsset;
class TextureAsset;

using MaterialAssetHandle = AssetHandle<MaterialAsset>;
using MaterialTemplateAssetHandle = AssetHandle<MaterialTemplateAsset>;
using MeshAssetHandle = AssetHandle<MeshAsset>;
using ModelAssetHandle = AssetHandle<ModelAsset>;
using ShaderAssetHandle = AssetHandle<ShaderAsset>;
using TextureAssetHandle = AssetHandle<TextureAsset>;

} // namespace rubia::asset
