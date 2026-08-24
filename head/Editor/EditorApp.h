#pragma once

#include "App.h"
#include "Editor/EditorLayer.h"

namespace VkRenderer
{

/// Editor composition root. Runtime App remains unaware of Editor classes.
class EditorApp final
{
public:
    void run();
    void runRenderTest();

private:
    [[nodiscard]] static App::RunConfig makeRunConfig();

    App app_;
    EditorLayer editorLayer_;
};

} // namespace VkRenderer
