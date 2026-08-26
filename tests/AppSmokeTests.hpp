#pragma once

#include "App.hpp"

namespace VkRenderer
{

class ApplicationGui;

namespace Test
{

/// Existing command-line validation paths, kept outside the runtime App API.
class AppSmokeTests final
{
public:
    static void runAssetImportTest();
    static void runRenderTest();
    static void runRenderTest(
        App& app,
        const App::RunConfig& config,
        ApplicationGui& gui);
};

} // namespace Test
} // namespace VkRenderer


