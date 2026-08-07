#include "Win32Window.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace Backend
{
namespace { constexpr wchar_t kClassName[] = L"CleanImguiD3D12Window"; }

bool Win32Window::Create(const wchar_t* title, std::uint32_t width, std::uint32_t height)
{
    m_instance = GetModuleHandleW(nullptr);
    m_width = width;
    m_height = height;

    WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_instance;
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    RECT rect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowW(kClassName, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, m_instance, this);
    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    return true;
}

void Win32Window::Destroy()
{
    if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
    if (m_instance) UnregisterClassW(kClassName, m_instance);
}

bool Win32Window::PumpMessages()
{
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

bool Win32Window::ConsumeResize() noexcept
{
    const bool pending = m_resizePending;
    m_resizePending = false;
    return pending;
}

LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win32Window* self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<Win32Window*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Win32Window::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;

    switch (msg)
    {
    case WM_SIZE:
        m_minimized = wParam == SIZE_MINIMIZED;
        if (!m_minimized)
        {
            m_width = LOWORD(lParam);
            m_height = HIWORD(lParam);
            m_resizePending = m_width > 0 && m_height > 0;
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
}
