#pragma once

#include "FrameContext.hpp"
#include <Windows.h>
#include <array>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace Backend
{
class D3D12Backend
{
public:
    static constexpr std::uint32_t FrameCount = 3;

    bool Initialize(HWND hwnd, std::uint32_t width, std::uint32_t height);
    void Shutdown();
    void Resize(std::uint32_t width, std::uint32_t height);
    void BeginFrame();
    void EndFrame();
    void RenderImGui();
    void WaitForGpu();

    ID3D12Device* Device() const noexcept { return m_device.Get(); }
    ID3D12DescriptorHeap* SrvHeap() const noexcept { return m_srvHeap.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE FontCpuHandle() const noexcept { return m_srvHeap->GetCPUDescriptorHandleForHeapStart(); }
    D3D12_GPU_DESCRIPTOR_HANDLE FontGpuHandle() const noexcept { return m_srvHeap->GetGPUDescriptorHandleForHeapStart(); }

private:
    bool CreateDevice();
    bool CreateSwapChain(HWND hwnd, std::uint32_t width, std::uint32_t height);
    bool CreateHeaps();
    bool CreateFrames();
    bool CreateRenderTargets();
    void ReleaseRenderTargets();
    void WaitForFrame(FrameContext& frame);

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> m_backBuffers;
    std::array<FrameContext, FrameCount> m_frames;
    HANDLE m_fenceEvent = nullptr;
    std::uint64_t m_nextFenceValue = 1;
    std::uint32_t m_rtvDescriptorSize = 0;
    std::uint32_t m_frameIndex = 0;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};
}
