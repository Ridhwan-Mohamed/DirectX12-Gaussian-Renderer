#pragma once

#include "Camera.h"
#include "D3D12Renderer.h"

#include <Windows.h>

#include <chrono>
#include <cstdint>

class Application
{
public:
    explicit Application(HINSTANCE instance) noexcept;
    int Run(int showCommand);

private:
    static constexpr wchar_t WindowClassName[] =
        L"DirectX12GaussianRendererWindow";

    static LRESULT CALLBACK WindowProcedure(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    LRESULT HandleMessage(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    void RegisterWindowClass();
    void CreateApplicationWindow();
    void UpdateInput(float deltaSeconds);
    void UpdateWindowTitle(double deltaSeconds);
    void ApplyResize();

    HINSTANCE instance_ = nullptr;
    HWND window_ = nullptr;
    D3D12Renderer renderer_;
    Camera camera_;

    std::uint32_t clientWidth_ = 1280;
    std::uint32_t clientHeight_ = 720;
    bool minimized_ = false;
    bool inSizeMove_ = false;
    bool rotatingCamera_ = false;
    POINT previousMousePosition_{};

    double statisticsSeconds_ = 0.0;
    std::uint32_t statisticsFrameCount_ = 0;
};
