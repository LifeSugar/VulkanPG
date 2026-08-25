#pragma once

#include "Asset/AssetHandle.h"

namespace VkRenderer
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

} // namespace VkRenderer
