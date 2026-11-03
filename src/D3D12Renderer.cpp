#include "D3D12Renderer.h"

#include "Win32Utils.h"

#include <chrono>
#include <limits>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D12_RESOURCE_BARRIER TransitionBarrier(
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after) noexcept
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        return barrier;
    }

    D3D12_HEAP_PROPERTIES ReadbackHeapProperties() noexcept
    {
        D3D12_HEAP_PROPERTIES properties{};
        properties.Type = D3D12_HEAP_TYPE_READBACK;
        properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        properties.CreationNodeMask = 1;
        properties.VisibleNodeMask = 1;
        return properties;
    }

    D3D12_RESOURCE_DESC BufferDescription(std::uint64_t byteCount) noexcept
    {
        D3D12_RESOURCE_DESC description{};
        description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        description.Alignment = 0;
        description.Width = byteCount;
        description.Height = 1;
        description.DepthOrArraySize = 1;
        description.MipLevels = 1;
        description.Format = DXGI_FORMAT_UNKNOWN;
        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;
        description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        description.Flags = D3D12_RESOURCE_FLAG_NONE;
        return description;
    }
}

D3D12Renderer::~D3D12Renderer()
{
    if (initialized_)
    {
        try
        {
            WaitForGpu();
        }
        catch (...)
        {
            // Destructors cannot report a recoverable failure. Device removal is
            // reported on the rendering path before shutdown whenever possible.
        }
    }

    if (timestampReadback_ && mappedTimestamps_ != nullptr)
    {
        D3D12_RANGE writtenRange{0, 0};
        timestampReadback_->Unmap(0, &writtenRange);
        mappedTimestamps_ = nullptr;
    }

    if (fenceEvent_ != nullptr)
    {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

void D3D12Renderer::Initialize(
    HWND window,
    std::uint32_t width,
    std::uint32_t height)
{
    if (initialized_)
    {
        throw std::logic_error("D3D12Renderer::Initialize called twice");
    }

    width_ = width;
    height_ = height;

    EnableDebugLayer();
    CreateFactoryAndDevice();
    CreateCommandObjects();
    CreateSwapChain(window);
    CreateRenderTargets();
    CreateSynchronizationObjects();
    CreateTimestampResources();

    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.Width = static_cast<float>(width_);
    viewport_.Height = static_cast<float>(height_);
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_ = {
        0,
        0,
        static_cast<LONG>(width_),
        static_cast<LONG>(height_)};

    initialized_ = true;
}

void D3D12Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }

    ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
    {
        dredSettings->SetAutoBreadcrumbsEnablement(
            D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->SetPageFaultEnablement(
            D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->SetBreadcrumbContextEnablement(
            D3D12_DRED_ENABLEMENT_FORCED_ON);
    }
#endif
}

void D3D12Renderer::CreateFactoryAndDevice()
{
    UINT factoryFlags = 0;
#if defined(_DEBUG)
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    win32::ThrowIfFailed(
        CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_)),
        "CreateDXGIFactory2");

    ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(factory_.As(&factory5)))
    {
        BOOL allowTearing = FALSE;
        if (SUCCEEDED(factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing,
                sizeof(allowTearing))))
        {
            tearingSupported_ = allowTearing == TRUE;
        }
    }

    for (UINT adapterIndex = 0;; ++adapterIndex)
    {
        ComPtr<IDXGIAdapter1> candidate;
        const HRESULT enumerationResult = factory_->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&candidate));

        if (enumerationResult == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }
        win32::ThrowIfFailed(
            enumerationResult,
            "IDXGIFactory7::EnumAdapterByGpuPreference");

        DXGI_ADAPTER_DESC1 description{};
        win32::ThrowIfFailed(
            candidate->GetDesc1(&description),
            "IDXGIAdapter1::GetDesc1");

        if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
        {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(
                candidate.Get(),
                D3D_FEATURE_LEVEL_12_0,
                __uuidof(ID3D12Device),
                nullptr)))
        {
            win32::ThrowIfFailed(
                candidate.As(&adapter_),
                "Query IDXGIAdapter4");
            break;
        }
    }

    if (!adapter_)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        win32::ThrowIfFailed(
            factory_->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)),
            "IDXGIFactory7::EnumWarpAdapter");
        win32::ThrowIfFailed(
            warpAdapter.As(&adapter_),
            "Query WARP IDXGIAdapter4");
    }

    DXGI_ADAPTER_DESC3 adapterDescription{};
    win32::ThrowIfFailed(
        adapter_->GetDesc3(&adapterDescription),
        "IDXGIAdapter4::GetDesc3");
    adapterName_ = adapterDescription.Description;

    win32::ThrowIfFailed(
        D3D12CreateDevice(
            adapter_.Get(),
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&device_)),
        "D3D12CreateDevice");

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device_.As(&infoQueue)))
    {
        infoQueue->SetBreakOnSeverity(
            D3D12_MESSAGE_SEVERITY_CORRUPTION,
            TRUE);
        infoQueue->SetBreakOnSeverity(
            D3D12_MESSAGE_SEVERITY_ERROR,
            TRUE);
    }
#endif
}

void D3D12Renderer::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDescription{};
    queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDescription.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDescription.NodeMask = 0;

    win32::ThrowIfFailed(
        device_->CreateCommandQueue(
            &queueDescription,
            IID_PPV_ARGS(&commandQueue_)),
        "ID3D12Device::CreateCommandQueue");
    commandQueue_->SetName(L"Main graphics queue");

    for (std::uint32_t index = 0; index < FrameCount; ++index)
    {
        win32::ThrowIfFailed(
            device_->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&frames_[index].commandAllocator)),
            "ID3D12Device::CreateCommandAllocator");
    }

    win32::ThrowIfFailed(
        device_->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            frames_[0].commandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList_)),
        "ID3D12Device::CreateCommandList");
    commandList_->SetName(L"Main graphics command list");
    win32::ThrowIfFailed(
        commandList_->Close(),
        "ID3D12GraphicsCommandList::Close (initial)");
}

void D3D12Renderer::CreateSwapChain(HWND window)
{
    DXGI_SWAP_CHAIN_DESC1 description{};
    description.Width = width_;
    description.Height = height_;
    description.Format = SwapChainFormat;
    description.Stereo = FALSE;
    description.SampleDesc.Count = 1;
    description.SampleDesc.Quality = 0;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = FrameCount;
    description.Scaling = DXGI_SCALING_STRETCH;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    description.Flags = tearingSupported_
        ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    win32::ThrowIfFailed(
        factory_->CreateSwapChainForHwnd(
            commandQueue_.Get(),
            window,
            &description,
            nullptr,
            nullptr,
            &swapChain1),
        "IDXGIFactory7::CreateSwapChainForHwnd");

    win32::ThrowIfFailed(
        factory_->MakeWindowAssociation(
            window,
            DXGI_MWA_NO_ALT_ENTER),
        "IDXGIFactory7::MakeWindowAssociation");
    win32::ThrowIfFailed(
        swapChain1.As(&swapChain_),
        "Query IDXGISwapChain4");

    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
    heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDescription.NumDescriptors = FrameCount;
    heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    win32::ThrowIfFailed(
        device_->CreateDescriptorHeap(
            &heapDescription,
            IID_PPV_ARGS(&renderTargetHeap_)),
        "ID3D12Device::CreateDescriptorHeap (RTV)");
    renderTargetDescriptorSize_ =
        device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void D3D12Renderer::CreateRenderTargets()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        renderTargetHeap_->GetCPUDescriptorHandleForHeapStart();

    for (std::uint32_t index = 0; index < FrameCount; ++index)
    {
        win32::ThrowIfFailed(
            swapChain_->GetBuffer(
                index,
                IID_PPV_ARGS(&renderTargets_[index])),
            "IDXGISwapChain4::GetBuffer");
        device_->CreateRenderTargetView(
            renderTargets_[index].Get(),
            nullptr,
            handle);

        handle.ptr += renderTargetDescriptorSize_;
    }
}

void D3D12Renderer::CreateSynchronizationObjects()
{
    win32::ThrowIfFailed(
        device_->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&fence_)),
        "ID3D12Device::CreateFence");

    fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent_ == nullptr)
    {
        win32::ThrowLastError("CreateEventW (fence)");
    }
}

void D3D12Renderer::CreateTimestampResources()
{
    D3D12_QUERY_HEAP_DESC queryDescription{};
    queryDescription.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    queryDescription.Count = FrameCount * 2;
    queryDescription.NodeMask = 0;

    win32::ThrowIfFailed(
        device_->CreateQueryHeap(
            &queryDescription,
            IID_PPV_ARGS(&timestampQueryHeap_)),
        "ID3D12Device::CreateQueryHeap (timestamp)");

    const std::uint64_t byteCount =
        sizeof(std::uint64_t) * FrameCount * 2;
    const D3D12_HEAP_PROPERTIES heapProperties =
        ReadbackHeapProperties();
    const D3D12_RESOURCE_DESC resourceDescription =
        BufferDescription(byteCount);

    win32::ThrowIfFailed(
        device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescription,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&timestampReadback_)),
        "ID3D12Device::CreateCommittedResource (timestamp readback)");

    D3D12_RANGE readRange{0, static_cast<SIZE_T>(byteCount)};
    win32::ThrowIfFailed(
        timestampReadback_->Map(
            0,
            &readRange,
            reinterpret_cast<void**>(&mappedTimestamps_)),
        "ID3D12Resource::Map (timestamp readback)");

    win32::ThrowIfFailed(
        commandQueue_->GetTimestampFrequency(&timestampFrequency_),
        "ID3D12CommandQueue::GetTimestampFrequency");
}

void D3D12Renderer::Render()
{
    if (!initialized_ || width_ == 0 || height_ == 0)
    {
        return;
    }

    const auto cpuStart = std::chrono::steady_clock::now();

    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    WaitForFrame(frameIndex_);
    ReadCompletedTimestamp(frameIndex_);

    FrameContext& frame = frames_[frameIndex_];
    win32::ThrowIfFailed(
        frame.commandAllocator->Reset(),
        "ID3D12CommandAllocator::Reset");
    win32::ThrowIfFailed(
        commandList_->Reset(frame.commandAllocator.Get(), nullptr),
        "ID3D12GraphicsCommandList::Reset");

    const std::uint32_t firstQuery = frameIndex_ * 2;
    commandList_->EndQuery(
        timestampQueryHeap_.Get(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        firstQuery);

    D3D12_RESOURCE_BARRIER toRenderTarget = TransitionBarrier(
        renderTargets_[frameIndex_].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList_->ResourceBarrier(1, &toRenderTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle =
        renderTargetHeap_->GetCPUDescriptorHandleForHeapStart();
    renderTargetHandle.ptr +=
        static_cast<SIZE_T>(frameIndex_) * renderTargetDescriptorSize_;

    commandList_->RSSetViewports(1, &viewport_);
    commandList_->RSSetScissorRects(1, &scissorRect_);
    commandList_->OMSetRenderTargets(
        1,
        &renderTargetHandle,
        FALSE,
        nullptr);

    constexpr float clearColour[4] = {
        0.018f,
        0.032f,
        0.065f,
        1.0f};
    commandList_->ClearRenderTargetView(
        renderTargetHandle,
        clearColour,
        0,
        nullptr);

    D3D12_RESOURCE_BARRIER toPresent = TransitionBarrier(
        renderTargets_[frameIndex_].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    commandList_->ResourceBarrier(1, &toPresent);

    commandList_->EndQuery(
        timestampQueryHeap_.Get(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        firstQuery + 1);
    commandList_->ResolveQueryData(
        timestampQueryHeap_.Get(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        firstQuery,
        2,
        timestampReadback_.Get(),
        static_cast<std::uint64_t>(firstQuery) * sizeof(std::uint64_t));

    win32::ThrowIfFailed(
        commandList_->Close(),
        "ID3D12GraphicsCommandList::Close");

    ID3D12CommandList* commandLists[] = {commandList_.Get()};
    commandQueue_->ExecuteCommandLists(1, commandLists);

    const UINT syncInterval = vsyncEnabled_ ? 1u : 0u;
    const UINT presentFlags =
        !vsyncEnabled_ && tearingSupported_
        ? DXGI_PRESENT_ALLOW_TEARING
        : 0u;
    const HRESULT presentResult = swapChain_->Present(
        syncInterval,
        presentFlags);

    if (FAILED(presentResult))
    {
        const HRESULT removalReason = device_->GetDeviceRemovedReason();
        if (FAILED(removalReason))
        {
            win32::ThrowHResult(
                removalReason,
                "D3D12 device removed during Present");
        }
        win32::ThrowHResult(presentResult, "IDXGISwapChain4::Present");
    }

    const std::uint64_t signalValue = nextFenceValue_++;
    win32::ThrowIfFailed(
        commandQueue_->Signal(fence_.Get(), signalValue),
        "ID3D12CommandQueue::Signal");
    frame.fenceValue = signalValue;
    timestampSubmitted_[frameIndex_] = true;

    const auto cpuEnd = std::chrono::steady_clock::now();
    lastCpuFrameMilliseconds_ =
        std::chrono::duration<double, std::milli>(cpuEnd - cpuStart).count();
}

void D3D12Renderer::Resize(
    std::uint32_t width,
    std::uint32_t height)
{
    if (!initialized_ || width == 0 || height == 0)
    {
        return;
    }

    if (width == width_ && height == height_)
    {
        return;
    }

    WaitForGpu();
    ReleaseRenderTargets();

    const UINT swapChainFlags = tearingSupported_
        ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        : 0;
    win32::ThrowIfFailed(
        swapChain_->ResizeBuffers(
            FrameCount,
            width,
            height,
            SwapChainFormat,
            swapChainFlags),
        "IDXGISwapChain4::ResizeBuffers");

    width_ = width;
    height_ = height;
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    for (FrameContext& frame : frames_)
    {
        frame.fenceValue = 0;
    }
    timestampSubmitted_.fill(false);

    CreateRenderTargets();

    viewport_.Width = static_cast<float>(width_);
    viewport_.Height = static_cast<float>(height_);
    scissorRect_.right = static_cast<LONG>(width_);
    scissorRect_.bottom = static_cast<LONG>(height_);
}

void D3D12Renderer::WaitForFrame(std::uint32_t frameIndex)
{
    WaitForFence(frames_[frameIndex].fenceValue);
}

void D3D12Renderer::WaitForFence(std::uint64_t fenceValue)
{
    if (fenceValue == 0 || fence_->GetCompletedValue() >= fenceValue)
    {
        return;
    }

    win32::ThrowIfFailed(
        fence_->SetEventOnCompletion(fenceValue, fenceEvent_),
        "ID3D12Fence::SetEventOnCompletion");
    const DWORD waitResult = WaitForSingleObject(fenceEvent_, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        win32::ThrowLastError("WaitForSingleObject (fence)");
    }
}

void D3D12Renderer::WaitForGpu()
{
    if (!commandQueue_ || !fence_)
    {
        return;
    }

    const std::uint64_t signalValue = nextFenceValue_++;
    win32::ThrowIfFailed(
        commandQueue_->Signal(fence_.Get(), signalValue),
        "ID3D12CommandQueue::Signal (flush)");
    WaitForFence(signalValue);
}

void D3D12Renderer::ReadCompletedTimestamp(
    std::uint32_t frameIndex) noexcept
{
    if (!timestampSubmitted_[frameIndex] ||
        mappedTimestamps_ == nullptr ||
        timestampFrequency_ == 0)
    {
        return;
    }

    const std::uint32_t firstQuery = frameIndex * 2;
    const std::uint64_t start = mappedTimestamps_[firstQuery];
    const std::uint64_t end = mappedTimestamps_[firstQuery + 1];
    if (end >= start)
    {
        lastGpuFrameMilliseconds_ =
            static_cast<double>(end - start) * 1000.0 /
            static_cast<double>(timestampFrequency_);
    }
}

void D3D12Renderer::ReleaseRenderTargets() noexcept
{
    for (ComPtr<ID3D12Resource>& renderTarget : renderTargets_)
    {
        renderTarget.Reset();
    }
}

void D3D12Renderer::SetVSyncEnabled(bool enabled) noexcept
{
    vsyncEnabled_ = enabled;
}

bool D3D12Renderer::VSyncEnabled() const noexcept
{
    return vsyncEnabled_;
}

bool D3D12Renderer::IsInitialized() const noexcept
{
    return initialized_;
}

double D3D12Renderer::CpuFrameMilliseconds() const noexcept
{
    return lastCpuFrameMilliseconds_;
}

double D3D12Renderer::GpuFrameMilliseconds() const noexcept
{
    return lastGpuFrameMilliseconds_;
}

const std::wstring& D3D12Renderer::AdapterName() const noexcept
{
    return adapterName_;
}
