#pragma once

namespace Backend { class Win32Window; class D3D12Backend; }

namespace Frontend
{
class Frontend
{
public:
    bool Initialize(Backend::Win32Window& window, Backend::D3D12Backend& graphics);
    void BeginFrame();
    void Render();
    void EndFrame(Backend::D3D12Backend& graphics);
    void Shutdown();

private:
    bool m_initialized = false;
};
}
