#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace rubia::asset
{

enum class ValidationSeverity
{
    Warning,
    Error
};

struct ValidationIssue
{
    ValidationSeverity severity = ValidationSeverity::Error;
    std::string code;
    std::string path;
    std::string message;
};

class ValidationReport final
{
public:
    void addWarning(
        std::string code,
        std::string path,
        std::string message);
    void addError(
        std::string code,
        std::string path,
        std::string message);
    void append(const ValidationReport& other);

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<ValidationIssue>& issues() const noexcept;
    [[nodiscard]] std::string toString() const;

private:
    std::vector<ValidationIssue> issues_;
};

class AssetValidationError final : public std::runtime_error
{
public:
    explicit AssetValidationError(ValidationReport report);

    [[nodiscard]] const ValidationReport& report() const noexcept;

private:
    ValidationReport report_;
};

} // namespace rubia::asset
