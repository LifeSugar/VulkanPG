#include "App.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
    VkRenderer::App app;

    try
    {
        if (argc == 2 && std::string_view(argv[1]) == "--asset-test")
        {
            app.runAssetImportTest();
            std::cout << "[OK] Asset import test passed\n";
        }
        else if (argc == 2 && std::string_view(argv[1]) == "--render-test")
        {
            app.runRenderTest();
            std::cout << "[OK] Render test passed\n";
        }
        else
        {
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
