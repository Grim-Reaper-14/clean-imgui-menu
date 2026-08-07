#include "D3D12Backend.hpp"

#include "../../../../../backends/imgui_impl_dx12.h"
#include "../../../../../imgui.h"

namespace Backend
{
using Microsoft::WRL::ComPtr;

bool D3D12Backend::Initialize(HWND hwnd, std::uint32_t width, std::uint32_t height)
{
    m_width = width; m_height = height;
    if (!CreateDevice() || !CreateHeaps() || !CreateSwapChain(hwnd, width, height) || !CreateFrames() || !CreateRenderTargets()) return false;
    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return m_fenceEvent != nullptr;
}

bool D3D12Backend::CreateDevice()
{
    UINT flags = 0;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) { debug->EnableDebugLayer(); flags |= DXGI_CREATE_FACTORY_DEBUG; }
#endif
    if (FAILED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)))) return false;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc{}; adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { adapter.Reset(); continue; }
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) break;
        adapter.Reset();
    }
    if (!m_device && FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) return false;

    D3D12_COMMAND_QUEUE_DESC queue{};
    queue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    return SUCCEEDED(m_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&m_commandQueue)));
}

bool D3D12Backend::CreateHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv{}; rtv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; rtv.NumDescriptors = FrameCount;
    if (FAILED(m_device->CreateDescriptorHeap(&rtv, IID_PPV_ARGS(&m_rtvHeap)))) return false;
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC srv{}; srv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; srv.NumDescriptors = 64; srv.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    return SUCCEEDED(m_device->CreateDescriptorHeap(&srv, IID_PPV_ARGS(&m_srvHeap)));
}

bool D3D12Backend::CreateSwapChain(HWND hwnd, std::uint32_t width, std::uint32_t height)
{
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = FrameCount; desc.Width = width; desc.Height = height; desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; desc.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> swap;
    if (FAILED(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, &desc, nullptr, nullptr, &swap))) return false;
    if (FAILED(swap.As(&m_swapChain))) return false;
    m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool D3D12Backend::CreateFrames()
{
    for (auto& frame : m_frames)
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.commandAllocator)))) return false;
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frames[0].commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)))) return false;
    m_commandList->Close();
    return SUCCEEDED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
}

bool D3D12Backend::CreateRenderTargets()
{
    auto handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (std::uint32_t i = 0; i < FrameCount; ++i)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])))) return false;
        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, handle);
        handle.ptr += m_rtvDescriptorSize;
    }
    return true;
}

void D3D12Backend::ReleaseRenderTargets() { for (auto& buffer : m_backBuffers) buffer.Reset(); }

void D3D12Backend::WaitForFrame(FrameContext& frame)
{
    if (!frame.fenceValue || m_fence->GetCompletedValue() >= frame.fenceValue) return;
    m_fence->SetEventOnCompletion(frame.fenceValue, m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);
}

void D3D12Backend::BeginFrame()
{
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    auto& frame = m_frames[m_frameIndex];
    WaitForFrame(frame);
    frame.commandAllocator->Reset();
    m_commandList->Reset(frame.commandAllocator.Get(), nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_backBuffers[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);

    auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart(); rtv.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
    constexpr float clear[4] = { 0.035f, 0.035f, 0.045f, 1.0f };
    m_commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    m_commandList->ClearRenderTargetView(rtv, clear, 0, nullptr);
}

void D3D12Backend::RenderImGui()
{
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
}

void D3D12Backend::EndFrame()
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_backBuffers[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);
    m_commandList->Close();

    ID3D12CommandList* lists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, lists);
    m_swapChain->Present(1, 0);

    auto& frame = m_frames[m_frameIndex];
    frame.fenceValue = m_nextFenceValue++;
    m_commandQueue->Signal(m_fence.Get(), frame.fenceValue);
}

void D3D12Backend::WaitForGpu()
{
    if (!m_commandQueue || !m_fence || !m_fenceEvent) return;
    const auto value = m_nextFenceValue++;
    m_commandQueue->Signal(m_fence.Get(), value);
    if (m_fence->GetCompletedValue() < value) { m_fence->SetEventOnCompletion(value, m_fenceEvent); WaitForSingleObject(m_fenceEvent, INFINITE); }
}

void D3D12Backend::Resize(std::uint32_t width, std::uint32_t height)
{
    if (!width || !height || !m_swapChain) return;
    WaitForGpu(); ReleaseRenderTargets();
    if (SUCCEEDED(m_swapChain->ResizeBuffers(FrameCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0))) { m_width = width; m_height = height; CreateRenderTargets(); }
}

void D3D12Backend::Shutdown()
{
    WaitForGpu(); ReleaseRenderTargets();
    if (m_fenceEvent) { CloseHandle(m_fenceEvent); m_fenceEvent = nullptr; }
    m_commandList.Reset(); m_fence.Reset(); m_srvHeap.Reset(); m_rtvHeap.Reset(); m_swapChain.Reset(); m_commandQueue.Reset(); m_device.Reset(); m_factory.Reset();
}
}
