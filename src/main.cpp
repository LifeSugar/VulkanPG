#include "VKApp.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    VulkanApp app;

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