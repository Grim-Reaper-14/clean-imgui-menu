#include "Application.hpp"

#include "../Backend/Window/Win32Window.hpp"
#include "../Backend/Graphics/D3D12/D3D12Backend.hpp"
#include "../Frontend/Frontend.hpp"
#include "../Threading_Module/MainThreadDispatcher.hpp"
#include "../Threading_Module/ThreadPool.hpp"

namespace Core
{
Application::Application() = default;
Application::~Application() { Shutdown(); }

bool Application::Initialize()
{
    m_dispatcher = std::make_unique<Threading::MainThreadDispatcher>();
    m_threadPool = std::make_unique<Threading::ThreadPool>();
    m_window = std::make_unique<Backend::Win32Window>();
    m_graphics = std::make_unique<Backend::D3D12Backend>();
    m_frontend = std::make_unique<Frontend::Frontend>();

    if (!m_window->Create(L"Clean ImGui D3D12", 1100, 720)) return false;
    if (!m_graphics->Initialize(m_window->Handle(), m_window->Width(), m_window->Height())) return false;
    if (!m_frontend->Initialize(*m_window, *m_graphics)) return false;
    return true;
}

int Application::Run()
{
    if (!Initialize()) return 1;

    while (m_running && m_window->PumpMessages())
        Tick();

    Shutdown();
    return 0;
}

void Application::Tick()
{
    m_dispatcher->Drain();

    if (m_window->ConsumeResize())
        m_graphics->Resize(m_window->Width(), m_window->Height());

    if (m_window->IsMinimized())
        return;

    m_graphics->BeginFrame();
    m_frontend->BeginFrame();
    m_frontend->Render();
    m_frontend->EndFrame(*m_graphics);
    m_graphics->EndFrame();
}

void Application::Shutdown()
{
    if (m_threadPool) m_threadPool->Stop();
    if (m_frontend) { m_frontend->Shutdown(); m_frontend.reset(); }
    if (m_graphics) { m_graphics->Shutdown(); m_graphics.reset(); }
    if (m_window) { m_window->Destroy(); m_window.reset(); }
    m_dispatcher.reset();
    m_threadPool.reset();
}
}
