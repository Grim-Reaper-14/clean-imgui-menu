#pragma once

#include <memory>

namespace Backend { class Win32Window; class D3D12Backend; }
namespace Frontend { class Frontend; }
namespace Threading { class ThreadPool; class MainThreadDispatcher; }

namespace Core
{
class Application
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int Run();
    void RequestExit() noexcept { m_running = false; }

private:
    bool Initialize();
    void Shutdown();
    void Tick();

    bool m_running = true;
    std::unique_ptr<Backend::Win32Window> m_window;
    std::unique_ptr<Backend::D3D12Backend> m_graphics;
    std::unique_ptr<Frontend::Frontend> m_frontend;
    std::unique_ptr<Threading::ThreadPool> m_threadPool;
    std::unique_ptr<Threading::MainThreadDispatcher> m_dispatcher;
};
}
