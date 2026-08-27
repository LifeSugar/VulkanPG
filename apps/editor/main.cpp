#include "App.hpp"
#include "EditorApp.hpp"
#include "AppSmokeTests.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
    try
    {
        if (argc == 2 && std::string_view(argv[1]) == "--asset-test")
        {
            rubia::test::AppSmokeTests::runAssetImportTest();
            std::cout << "[OK] Asset import test passed\n";
        }
        else if (argc == 2 && std::string_view(argv[1]) == "--render-test")
        {
            rubia::test::AppSmokeTests::runRenderTest();
            std::cout << "[OK] Render test passed\n";
        }
        else if (argc == 2 && std::string_view(argv[1]) == "--editor-test")
        {
            rubia::editor::EditorApp editor;
            editor.runRenderTest();
            std::cout << "[OK] Editor render test passed\n";
        }
        else if (argc == 2 && std::string_view(argv[1]) == "--editor")
        {
            rubia::editor::EditorApp editor;
            editor.run();
        }
        else
        {
            rubia::editor::App app;
            app.run();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
