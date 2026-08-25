#include "Asset/ValidationReport.h"

#include <sstream>
#include <utility>

namespace VkRenderer
{

void ValidationReport::addWarning(
    std::string code,
    std::string path,
    std::string message)
{
    issues_.push_back({
        ValidationSeverity::Warning,
        std::move(code),
        std::move(path),
        std::move(message)
    });
}

void ValidationReport::addError(
    std::string code,
    std::string path,
    std::string message)
{
    issues_.push_back({
        ValidationSeverity::Error,
        std::move(code),
        std::move(path),
        std::move(message)
    });
}

void ValidationReport::append(const ValidationReport& other)
{
    issues_.insert(
        issues_.end(),
        other.issues_.begin(),
        other.issues_.end());
}

bool ValidationReport::valid() const noexcept
{
    for (const ValidationIssue& issue : issues_)
    {
        if (issue.severity == ValidationSeverity::Error)
        {
            return false;
        }
    }
    return true;
}

bool ValidationReport::empty() const noexcept
{
    return issues_.empty();
}

const std::vector<ValidationIssue>& ValidationReport::issues() const noexcept
{
    return issues_;
}

std::string ValidationReport::toString() const
{
    std::ostringstream stream;
    for (const ValidationIssue& issue : issues_)
    {
        stream
            << (issue.severity == ValidationSeverity::Error
                    ? "error"
                    : "warning")
            << " [" << issue.code << ']';
        if (!issue.path.empty())
        {
            stream << " " << issue.path;
        }
        if (!issue.message.empty())
        {
            stream << ": " << issue.message;
        }
        stream << '\n';
    }
    return stream.str();
}

AssetValidationError::AssetValidationError(ValidationReport report)
    : std::runtime_error(report.toString()),
      report_(std::move(report))
{
}

const ValidationReport& AssetValidationError::report() const noexcept
{
    return report_;
}

} // namespace VkRenderer
