#pragma once

#include <Windows.h>
#include <cstdint>

namespace Backend
{
class Win32Window
{
public:
    bool Create(const wchar_t* title, std::uint32_t width, std::uint32_t height);
    void Destroy();
    bool PumpMessages();

    HWND Handle() const noexcept { return m_hwnd; }
    std::uint32_t Width() const noexcept { return m_width; }
    std::uint32_t Height() const noexcept { return m_height; }
    bool IsMinimized() const noexcept { return m_minimized; }
    bool ConsumeResize() noexcept;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    HINSTANCE m_instance = nullptr;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    bool m_resizePending = false;
    bool m_minimized = false;
};
}
