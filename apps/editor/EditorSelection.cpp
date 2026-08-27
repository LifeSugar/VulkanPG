#include "EditorSelection.hpp"

#include <utility>

namespace rubia::editor
{

void EditorSelection::select(InspectorTarget target)
{
    target_ = std::move(target);
    history_.clear();
}

void EditorSelection::navigateTo(InspectorTarget target)
{
    if (!empty())
    {
        history_.push_back(target_);
    }
    target_ = std::move(target);
}

bool EditorSelection::canNavigateBack() const noexcept
{
    return !history_.empty();
}

void EditorSelection::navigateBack()
{
    if (history_.empty())
    {
        return;
    }
    target_ = std::move(history_.back());
    history_.pop_back();
}

void EditorSelection::clear() noexcept
{
    target_ = std::monostate{};
    history_.clear();
}

const InspectorTarget& EditorSelection::target() const noexcept
{
    return target_;
}

bool EditorSelection::empty() const noexcept
{
    return std::holds_alternative<std::monostate>(target_);
}

} // namespace rubia::editor
