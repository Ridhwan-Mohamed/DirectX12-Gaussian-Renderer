#include "Application.h"

#include <Windows.h>

#include <cstdlib>
#include <exception>

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    try
    {
        Application application(instance);
        return application.Run(showCommand);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(
            nullptr,
            exception.what(),
            "DirectX 12 Gaussian Renderer",
            MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
}
