#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <string>
#include <span>

#include "GpuGaussian.h"

class D3D12Renderer
{
public:
    static constexpr std::uint32_t FrameCount = 3;

    D3D12Renderer() = default;
    ~D3D12Renderer();

    D3D12Renderer(const D3D12Renderer&) = delete;
    D3D12Renderer& operator=(const D3D12Renderer&) = delete;

    void Initialize(
        HWND window,
        std::uint32_t width,
        std::uint32_t height);
    void UploadGaussianScene(
        std::span<const GpuGaussian> gaussians);
    void Resize(std::uint32_t width, std::uint32_t height);
    void Render();
    void WaitForGpu();

    void SetVSyncEnabled(bool enabled) noexcept;
    [[nodiscard]] bool VSyncEnabled() const noexcept;
    [[nodiscard]] bool IsInitialized() const noexcept;
    [[nodiscard]] double CpuFrameMilliseconds() const noexcept;
    [[nodiscard]] double GpuFrameMilliseconds() const noexcept;
    [[nodiscard]] const std::wstring& AdapterName() const noexcept;
    [[nodiscard]] std::uint32_t GaussianCount() const noexcept;
    [[nodiscard]] std::uint64_t GaussianBufferSizeBytes() const noexcept;

private:
    struct FrameContext
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        std::uint64_t fenceValue = 0;
    };

    void EnableDebugLayer();
    void CreateFactoryAndDevice();
    void CreateCommandObjects();
    void CreateSwapChain(HWND window);
    void CreateRenderTargets();
    void CreateSynchronizationObjects();
    void CreateTimestampResources();
    void WaitForFrame(std::uint32_t frameIndex);
    void WaitForFence(std::uint64_t fenceValue);
    void ReadCompletedTimestamp(std::uint32_t frameIndex) noexcept;
    void ReleaseRenderTargets() noexcept;
    void CreateGaussianShaderResourceView();

    Microsoft::WRL::ComPtr<IDXGIFactory7> factory_;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetHeap_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> timestampQueryHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> timestampReadback_;
    Microsoft::WRL::ComPtr<ID3D12Resource> gaussianBuffer_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> shaderResourceHeap_;
    std::uint32_t gaussianCount_ = 0;
    std::uint64_t gaussianBufferSizeBytes_ = 0;

    std::array<FrameContext, FrameCount> frames_{};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount>
        renderTargets_{};
    std::array<bool, FrameCount> timestampSubmitted_{};

    std::uint64_t* mappedTimestamps_ = nullptr;
    std::uint64_t timestampFrequency_ = 0;
    std::uint64_t nextFenceValue_ = 1;
    HANDLE fenceEvent_ = nullptr;

    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
    std::uint32_t renderTargetDescriptorSize_ = 0;
    std::uint32_t frameIndex_ = 0;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    bool tearingSupported_ = false;
    bool vsyncEnabled_ = true;
    bool initialized_ = false;

    double lastCpuFrameMilliseconds_ = 0.0;
    double lastGpuFrameMilliseconds_ = 0.0;
    std::wstring adapterName_ = L"Unknown adapter";
};
