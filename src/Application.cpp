#include "Application.h"

#include "PlyGaussianLoader.h"
#include "Win32Utils.h"
#include "GpuGaussian.h"

#include <windowsx.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

Application::Application(
    HINSTANCE instance,
    std::filesystem::path scenePath)
    : instance_(instance),
      camera_(static_cast<float>(clientWidth_) /
              static_cast<float>(clientHeight_)),
      scenePath_(std::move(scenePath))
{
}

int Application::Run(int showCommand)
{
    RegisterWindowClass();
    CreateApplicationWindow();

    scene_ = PlyGaussianLoader::Load(scenePath_);
    const std::string sceneReport = BuildGaussianSceneReport(scene_);
    OutputDebugStringA(sceneReport.c_str());

    renderer_.Initialize(window_, clientWidth_, clientHeight_);

    {
        const std::vector<GpuGaussian> gpuGaussians =
            PackGaussianScene(scene_);

        renderer_.UploadGaussianScene(gpuGaussians);
    }

    const double gaussianBufferMiB =
        static_cast<double>(renderer_.GaussianBufferSizeBytes()) /
        (1024.0 * 1024.0);
    std::ostringstream uploadReport;
    uploadReport << "Gaussian GPU scene uploaded\n"
                 << "  count: " << renderer_.GaussianCount() << '\n'
                 << "  stride: " << sizeof(GpuGaussian) << " bytes\n"
                 << "  resident buffer: "
                 << renderer_.GaussianBufferSizeBytes() << " bytes ("
                 << std::fixed << std::setprecision(2)
                 << gaussianBufferMiB << " MiB)\n"
                 << "  packed CPU staging: released after upload\n"
                 << "  upload staging: released after fence completion\n"
                 << "  SRV: one shader-visible structured-buffer descriptor\n";
    OutputDebugStringA(uploadReport.str().c_str());

    ShowWindow(window_, showCommand);
    UpdateWindow(window_);

    using Clock = std::chrono::steady_clock;
    auto previousTime = Clock::now();

    MSG message{};
    bool running = true;
    while (running)
    {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                running = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (!running)
        {
            break;
        }

        if (minimized_ || inSizeMove_)
        {
            WaitMessage();
            previousTime = Clock::now();
            continue;
        }

        const auto currentTime = Clock::now();
        const double rawDeltaSeconds =
            std::chrono::duration<double>(currentTime - previousTime).count();
        previousTime = currentTime;
        const float deltaSeconds = static_cast<float>(
            std::min(rawDeltaSeconds, 0.1));

        UpdateInput(deltaSeconds);
        renderer_.Render();
        UpdateWindowTitle(rawDeltaSeconds);
    }

    renderer_.WaitForGpu();
    return static_cast<int>(message.wParam);
}

void Application::RegisterWindowClass()
{
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &Application::WindowProcedure;
    windowClass.hInstance = instance_;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = WindowClassName;

    if (RegisterClassExW(&windowClass) == 0)
    {
        win32::ThrowLastError("RegisterClassExW");
    }
}

void Application::CreateApplicationWindow()
{
    RECT windowRectangle{
        0,
        0,
        static_cast<LONG>(clientWidth_),
        static_cast<LONG>(clientHeight_)};
    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    constexpr DWORD extendedStyle = 0;
    if (!AdjustWindowRectEx(
            &windowRectangle,
            style,
            FALSE,
            extendedStyle))
    {
        win32::ThrowLastError("AdjustWindowRectEx");
    }

    const int windowWidth = windowRectangle.right - windowRectangle.left;
    const int windowHeight = windowRectangle.bottom - windowRectangle.top;

    window_ = CreateWindowExW(
        extendedStyle,
        WindowClassName,
        L"DirectX 12 Gaussian Renderer",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        instance_,
        this);

    if (window_ == nullptr)
    {
        win32::ThrowLastError("CreateWindowExW");
    }
}

LRESULT CALLBACK Application::WindowProcedure(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    Application* application = reinterpret_cast<Application*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));

    if (message == WM_NCCREATE)
    {
        const auto* createData =
            reinterpret_cast<const CREATESTRUCTW*>(lParam);
        application = static_cast<Application*>(createData->lpCreateParams);
        SetWindowLongPtrW(
            window,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(application));
    }

    if (application != nullptr)
    {
        return application->HandleMessage(
            window,
            message,
            wParam,
            lParam);
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Application::HandleMessage(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        clientWidth_ = static_cast<std::uint32_t>(LOWORD(lParam));
        clientHeight_ = static_cast<std::uint32_t>(HIWORD(lParam));
        minimized_ =
            wParam == SIZE_MINIMIZED ||
            clientWidth_ == 0 ||
            clientHeight_ == 0;

        if (!minimized_ && !inSizeMove_)
        {
            ApplyResize();
        }
        return 0;

    case WM_ENTERSIZEMOVE:
        inSizeMove_ = true;
        return 0;

    case WM_EXITSIZEMOVE:
        inSizeMove_ = false;
        ApplyResize();
        return 0;

    case WM_DPICHANGED:
    {
        const RECT* suggestedRectangle =
            reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(
            window,
            nullptr,
            suggestedRectangle->left,
            suggestedRectangle->top,
            suggestedRectangle->right - suggestedRectangle->left,
            suggestedRectangle->bottom - suggestedRectangle->top,
            SWP_NOACTIVATE | SWP_NOZORDER);
        return 0;
    }

    case WM_GETMINMAXINFO:
    {
        auto* minimumMaximum = reinterpret_cast<MINMAXINFO*>(lParam);
        minimumMaximum->ptMinTrackSize.x = 640;
        minimumMaximum->ptMinTrackSize.y = 360;
        return 0;
    }

    case WM_RBUTTONDOWN:
        rotatingCamera_ = true;
        previousMousePosition_ = {
            GET_X_LPARAM(lParam),
            GET_Y_LPARAM(lParam)};
        SetCapture(window);
        return 0;

    case WM_RBUTTONUP:
        rotatingCamera_ = false;
        if (GetCapture() == window)
        {
            ReleaseCapture();
        }
        return 0;

    case WM_MOUSEMOVE:
        if (rotatingCamera_)
        {
            const POINT currentMousePosition{
                GET_X_LPARAM(lParam),
                GET_Y_LPARAM(lParam)};
            const float yawDelta =
                static_cast<float>(currentMousePosition.x -
                                   previousMousePosition_.x) *
                0.003f;
            const float pitchDelta =
                static_cast<float>(previousMousePosition_.y -
                                   currentMousePosition.y) *
                0.003f;
            camera_.Rotate(yawDelta, pitchDelta);
            previousMousePosition_ = currentMousePosition;
        }
        return 0;

    case WM_KILLFOCUS:
        rotatingCamera_ = false;
        if (GetCapture() == window)
        {
            ReleaseCapture();
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(window);
            return 0;
        }

        if (wParam == 'V' && (lParam & (1ll << 30)) == 0)
        {
            renderer_.SetVSyncEnabled(!renderer_.VSyncEnabled());
            return 0;
        }
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

void Application::UpdateInput(float deltaSeconds)
{
    if (GetForegroundWindow() != window_)
    {
        return;
    }

    const auto isDown = [](int key)
    {
        return (GetAsyncKeyState(key) & 0x8000) != 0;
    };

    float speed = 2.0f;
    if (isDown(VK_SHIFT))
    {
        speed *= 4.0f;
    }
    const float distance = speed * deltaSeconds;

    const float right =
        (isDown('D') ? distance : 0.0f) -
        (isDown('A') ? distance : 0.0f);
    const float up =
        (isDown('E') ? distance : 0.0f) -
        (isDown('Q') ? distance : 0.0f);
    const float forward =
        (isDown('W') ? distance : 0.0f) -
        (isDown('S') ? distance : 0.0f);

    camera_.MoveLocal(right, up, forward);
}

void Application::UpdateWindowTitle(double deltaSeconds)
{
    statisticsSeconds_ += deltaSeconds;
    ++statisticsFrameCount_;

    if (statisticsSeconds_ < 0.25)
    {
        return;
    }

    const double framesPerSecond =
        static_cast<double>(statisticsFrameCount_) /
        statisticsSeconds_;
    const DirectX::XMFLOAT3 position = camera_.Position();
    const double gaussianBufferMiB =
        static_cast<double>(renderer_.GaussianBufferSizeBytes()) /
        (1024.0 * 1024.0);

    std::wostringstream title;
    title << L"DirectX 12 Gaussian Renderer | "
          << renderer_.AdapterName()
          << L" | " << scene_.gaussians.size() << L" Gaussians"
          << L" | Scene " << std::fixed << std::setprecision(2)
          << gaussianBufferMiB << L" MiB"
          << L" | " << std::setprecision(1)
          << framesPerSecond << L" FPS"
          << L" | CPU " << std::setprecision(2)
          << renderer_.CpuFrameMilliseconds() << L" ms"
          << L" | GPU "
          << renderer_.GpuFrameMilliseconds() << L" ms"
          << L" | VSync "
          << (renderer_.VSyncEnabled() ? L"on" : L"off")
          << L" | Camera ("
          << std::setprecision(1)
          << position.x << L", "
          << position.y << L", "
          << position.z << L")";

    SetWindowTextW(window_, title.str().c_str());
    statisticsSeconds_ = 0.0;
    statisticsFrameCount_ = 0;
}

void Application::ApplyResize()
{
    if (minimized_ || clientWidth_ == 0 || clientHeight_ == 0)
    {
        return;
    }

    camera_.SetAspectRatio(
        static_cast<float>(clientWidth_) /
        static_cast<float>(clientHeight_));

    if (renderer_.IsInitialized())
    {
        renderer_.Resize(clientWidth_, clientHeight_);
    }
}
