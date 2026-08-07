#include "Frontend.hpp"

#include "../Backend/Window/Win32Window.hpp"
#include "../Backend/Graphics/D3D12/D3D12Backend.hpp"
#include "../../../imgui.h"
#include "../../../backends/imgui_impl_dx12.h"
#include "../../../backends/imgui_impl_win32.h"

namespace Frontend
{
bool Frontend::Initialize(Backend::Win32Window& window, Backend::D3D12Backend& graphics)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(window.Handle())) return false;
    if (!ImGui_ImplDX12_Init(graphics.Device(), Backend::D3D12Backend::FrameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM, graphics.SrvHeap(), graphics.FontCpuHandle(), graphics.FontGpuHandle()))
    {
        ImGui_ImplWin32_Shutdown();
        return false;
    }
    m_initialized = true;
    return true;
}

void Frontend::BeginFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Frontend::Render()
{
    ImGui::SetNextWindowSize(ImVec2(620.0f, 390.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Clean D3D12 Menu");
    ImGui::TextUnformatted("D3D12 modular foundation is running.");
    ImGui::Separator();
    ImGui::TextUnformatted("Core / Backend / Frontend / Threading_Module");
    ImGui::Spacing();
    ImGui::TextDisabled("Next: theme, custom widgets, config services and Lua lifecycle.");
    ImGui::End();
}

void Frontend::EndFrame(Backend::D3D12Backend& graphics)
{
    ImGui::Render();
    graphics.RenderImGui();
}

void Frontend::Shutdown()
{
    if (!m_initialized) return;
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}
}
