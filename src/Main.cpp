#include "Application.h"

#include <Windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <stdexcept>

namespace
{
    std::filesystem::path ExecutableDirectory()
    {
        std::wstring path(32'768, L'\0');
        const DWORD length = GetModuleFileNameW(
            nullptr,
            path.data(),
            static_cast<DWORD>(path.size()));
        if (length == 0 || length == path.size())
        {
            throw std::runtime_error(
                "Could not determine the executable directory");
        }
        path.resize(length);
        return std::filesystem::path(path).parent_path();
    }

    std::filesystem::path ScenePathFromCommandLine()
    {
        int argumentCount = 0;
        PWSTR* arguments = CommandLineToArgvW(
            GetCommandLineW(),
            &argumentCount);
        if (arguments == nullptr)
        {
            throw std::runtime_error("CommandLineToArgvW failed");
        }

        std::filesystem::path scenePath;
        if (argumentCount >= 2)
        {
            scenePath = arguments[1];
        }
        else
        {
            scenePath =
                ExecutableDirectory() /
                L"assets" /
                L"scenes" /
                L"anchor-splat-demo" /
                L"point_cloud.ply";
        }

        LocalFree(arguments);
        return scenePath;
    }
}

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
        Application application(instance, ScenePathFromCommandLine());
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
