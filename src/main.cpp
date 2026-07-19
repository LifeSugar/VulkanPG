#include "App.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    VkRenderer::App app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
