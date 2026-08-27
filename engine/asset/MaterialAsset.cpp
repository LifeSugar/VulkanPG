#include "asset/MaterialAsset.hpp"

#include <utility>

namespace rubia::asset
{

MaterialAsset::MaterialAsset(CompiledCreateInfo createInfo)
    : name_(std::move(createInfo.name)),
      materialTemplate_(createInfo.materialTemplate),
      renderState_(createInfo.renderState),
      parameterData_(std::move(createInfo.parameterData)),
      textures_(std::move(createInfo.textures))
{
}

void MaterialAsset::reset() noexcept
{
    name_.clear();
    materialTemplate_ = {};
    renderState_ = {};
    parameterData_.clear();
    textures_.clear();
}

} // namespace rubia::asset
