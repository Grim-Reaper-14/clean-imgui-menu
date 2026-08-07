#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace Backend
{
struct FrameContext
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    std::uint64_t fenceValue = 0;
};
}
