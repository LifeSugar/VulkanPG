#pragma once

#include "App.hpp"

#include <filesystem>

namespace rubia::editor
{
class ApplicationGui;
class App;
}

namespace rubia::test
{

/// Existing command-line validation paths, kept outside the runtime App API.
class AppSmokeTests final
{
public:
    /// CPU-only validation of SPIR-V import and reflected ShaderInterface data.
    static void runShaderAssetTest(const std::filesystem::path& shaderPath);
    static void runRenderTest();
    static void runRenderTest(
        editor::App& app,
        const editor::App::RunConfig& config,
        editor::ApplicationGui& gui);
};

} // namespace rubia::test
