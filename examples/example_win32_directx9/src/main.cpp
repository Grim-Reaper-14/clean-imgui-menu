#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include <dxsdk-d3dx/d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

#include <tchar.h>

#include <filesystem>
#include <string>
#include <system_error>

#include "background_pic.h"
#include "blur.hpp"
#include "config/ConfigManager.hpp"
#include "menu/VehicleMenu.hpp"

#include "segue_font.h"
#include "ico_font.h"

#include "Lua_Manager.hpp"

namespace
{
    constexpr int kMenuWidth = 905;
    constexpr int kMenuHeight = 624;
    constexpr DWORD kWindowStyle = WS_POPUP;

    std::filesystem::path GetExecutableDirectory()
    {
        std::wstring modulePath(MAX_PATH, L'\0');

        for (;;)
        {
            const DWORD length = ::GetModuleFileNameW(
                nullptr, modulePath.data(),
                static_cast<DWORD>(modulePath.size()));

            if (length == 0)
                return std::filesystem::current_path();

            if (length < modulePath.size())
            {
                modulePath.resize(length);
                return std::filesystem::path(modulePath).parent_path();
            }

            modulePath.resize(modulePath.size() * 2);
        }
    }

    void DrawPlaceholderPage(const char* title, float animationOffset)
    {
        ImGui::BeginChild(title, ImVec2(691.0f, 523.0f), true);
        ImGui::TextUnformatted(title);
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped(
            "This page is ready for its BigBaseV2 backend module. "
            "UI state should remain separate from game-thread operations.");
        ImGui::EndChild();
        (void)animationOffset;
    }
}

float color_edit4[4] = { 1.00f, 1.00f, 1.00f, 1.000f };
float accent_color[4] = { 0.745f, 0.151f, 0.151f, 1.000f };

static MenuSettings menu_settings;
static VehicleMenu vehicle_menu;
static const std::filesystem::path executable_directory =
    GetExecutableDirectory();
static const std::string settings_path =
    (executable_directory / L"setting.json").string();
static const std::string scripts_path =
    (executable_directory / L"scripts").string();
static const std::filesystem::path vehicle_preview_path =
    executable_directory / L"Images" / L"Vehicles";

bool active = false;
float size_child = 0.0f;
bool menu = true;

ImFont* ico = nullptr;
ImFont* ico_combo = nullptr;
ImFont* ico_button = nullptr;
ImFont* ico_grande = nullptr;
ImFont* segu = nullptr;
ImFont* default_segu = nullptr;
ImFont* bold_segu = nullptr;

LPDIRECT3D9 g_pD3D = nullptr;
LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
D3DPRESENT_PARAMETERS g_d3dpp = {};
IDirect3DTexture9* scene = nullptr;

int tab_count = 0;
int tabs = 0;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**)
{
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
        nullptr, nullptr, nullptr, nullptr, L"ReaperzMenuWindow", nullptr
    };
    ::RegisterClassExW(&wc);

    RECT windowRect = { 0, 0, kMenuWidth, kMenuHeight };
    ::AdjustWindowRectEx(&windowRect, kWindowStyle, FALSE, 0);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"Reaperz BigBaseV2 Menu",
        kWindowStyle,
        200,
        200,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromMemoryTTF(
        &seguoe, sizeof seguoe, 22, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    default_segu = io.Fonts->AddFontFromMemoryTTF(
        &seguoe, sizeof seguoe, 22, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    segu = io.Fonts->AddFontFromMemoryTTF(
        &seguoe, sizeof seguoe, 40, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    bold_segu = io.Fonts->AddFontFromMemoryTTF(
        &bold_segue, sizeof bold_segue, 40, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ico = io.Fonts->AddFontFromMemoryTTF(
        &icon, sizeof icon, 24, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ico_combo = io.Fonts->AddFontFromMemoryTTF(
        &icon, sizeof icon, 19, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ico_button = io.Fonts->AddFontFromMemoryTTF(
        &icon, sizeof icon, 25, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ico_grande = io.Fonts->AddFontFromMemoryTTF(
        &icon, sizeof icon, 40, nullptr, io.Fonts->GetGlyphRangesCyrillic());

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    vehicle_menu.Initialize(g_pd3dDevice, vehicle_preview_path);

    std::string config_status =
        "No setting.json found beside the executable; using defaults.";
    std::error_code settings_error;
    if (std::filesystem::exists(settings_path, settings_error))
    {
        std::string error;
        if (ConfigManager::Load(
                settings_path, menu_settings, color_edit4, accent_color, error))
        {
            config_status =
                "Loaded setting.json automatically from beside the executable.";
        }
        else
        {
            config_status = "Automatic load failed: " + error;
            ::OutputDebugStringA((config_status + "\n").c_str());
        }
    }
    else if (settings_error)
    {
        config_status = "Automatic load failed: " + settings_error.message();
        ::OutputDebugStringA((config_status + "\n").c_str());
    }

    const ImVec4 clear_color(0.45f, 0.55f, 0.60f, 1.00f);
    bool done = false;

    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBackground;

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(static_cast<float>(kMenuWidth), static_cast<float>(kMenuHeight)),
            ImGuiCond_Always);

        if (ImGui::Begin("Reaperz", &menu, windowFlags))
        {
            const ImVec2 windowPosition = ImGui::GetWindowPos();
            ImDrawList* windowDrawList = ImGui::GetWindowDrawList();
            ImDrawList* backgroundDrawList = ImGui::GetBackgroundDrawList();

            DrawBackgroundBlur(windowDrawList, g_pd3dDevice);

            if (scene == nullptr)
            {
                D3DXCreateTextureFromFileInMemoryEx(
                    g_pd3dDevice,
                    &background,
                    sizeof(background),
                    1920,
                    1080,
                    D3DX_DEFAULT,
                    0,
                    D3DFMT_UNKNOWN,
                    D3DPOOL_MANAGED,
                    D3DX_DEFAULT,
                    D3DX_DEFAULT,
                    0,
                    nullptr,
                    nullptr,
                    &scene);
            }

            if (scene != nullptr)
            {
                backgroundDrawList->AddImage(
                    scene,
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1920.0f, 1080.0f),
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1.0f, 1.0f),
                    ImColor(color_edit4[0], color_edit4[1], color_edit4[2], color_edit4[3]));
            }

            backgroundDrawList->AddRectFilled(
                windowPosition,
                ImVec2(kMenuWidth + windowPosition.x, kMenuHeight + windowPosition.y),
                ImColor(9, 9, 9, 180),
                10.0f);

            windowDrawList->AddRectFilled(
                ImVec2(189.0f + windowPosition.x, 75.0f + windowPosition.y),
                ImVec2(903.0f + windowPosition.x, 76.0f + windowPosition.y),
                ImColor(25, 25, 25, 180),
                10.0f);

            ImGui::SetCursorPos(ImVec2(800.0f, 21.0f));
            ImGui::OptButton("L", ImVec2(30.0f, 30.0f), false);
            ImGui::SameLine(840.0f);
            ImGui::OptButton("B", ImVec2(30.0f, 30.0f), true);

            windowDrawList->AddRectFilled(
                windowPosition,
                ImVec2(190.0f + windowPosition.x, kMenuHeight + windowPosition.y),
                ImGui::GetColorU32(ImGuiCol_ChildBg),
                10.0f,
                ImDrawFlags_RoundCornersLeft);

            const int firstGradientVertex = windowDrawList->VtxBuffer.Size;
            windowDrawList->AddText(
                ico_grande,
                40.0f,
                ImVec2(20.0f + windowPosition.x, 20.0f + windowPosition.y),
                ImColor(0.60f, 0.60f, 0.60f, 0.70f),
                "U");
            windowDrawList->AddText(
                bold_segu,
                40.0f,
                ImVec2(70.0f + windowPosition.x, 15.0f + windowPosition.y),
                ImColor(0.60f, 0.60f, 0.60f, 0.70f),
                "Reaperz");
            windowDrawList->AddRectFilled(
                ImVec2(70.0f + windowPosition.x, 51.0f + windowPosition.y),
                ImVec2(163.0f + windowPosition.x, 52.0f + windowPosition.y),
                ImColor(0.60f, 0.60f, 0.60f, 0.70f),
                10.0f);
            const int secondGradientVertex = windowDrawList->VtxBuffer.Size;
            ImGui::ShadeVertsLinearColorGradientKeepAlpha(
                windowDrawList,
                firstGradientVertex,
                secondGradientVertex,
                windowPosition,
                ImVec2(200.0f + windowPosition.x, 20.0f + windowPosition.y),
                ImColor(0.25f, 0.25f, 0.25f, 0.50f),
                ImColor(0.60f, 0.60f, 0.60f, 1.00f));

            ImGui::SetCursorPosY(80.0f);
            const auto SelectTab = [](const char* iconText, const char* label, int index)
            {
                if (ImGui::TabButton(iconText, label, ImVec2(190.0f, 40.0f)) &&
                    tab_count != index)
                {
                    tab_count = index;
                    active = true;
                }
            };

            SelectTab("P", "Self", 0);
            SelectTab("N", "Weapons", 1);
            SelectTab("Q", "Teleport", 2);
            SelectTab("I", "Visuals", 3);
            SelectTab("O", "Misc", 4);
            SelectTab("R", "Players", 5);
            SelectTab("T", "Recovery", 6);
            SelectTab("V", "Vehicle", 7);
            SelectTab("J", "Lua", 8);
            SelectTab("S", "Config", 9);

            if (active)
            {
                if (size_child <= 10.0f)
                    size_child += 1.0f / ImGui::GetIO().Framerate * 60.0f;
                else
                {
                    active = false;
                    tabs = tab_count;
                }
            }
            else if (size_child >= 0.0f)
            {
                size_child -= 1.0f / ImGui::GetIO().Framerate * 60.0f;
            }

            windowDrawList->AddCircleFilled(
                ImVec2(57.0f + windowPosition.x, 570.0f + windowPosition.y),
                25.0f,
                ImColor(10, 9, 10, 255),
                30);
            windowDrawList->AddCircle(
                ImVec2(57.0f + windowPosition.x, 570.0f + windowPosition.y),
                27.0f,
                ImColor(20, 19, 20, 255),
                30,
                4.0f);
            windowDrawList->AddText(
                segu,
                40.0f,
                ImVec2(51.0f + windowPosition.x, 548.0f + windowPosition.y),
                ImColor(0.60f, 0.60f, 0.60f, 0.50f),
                "?");
            windowDrawList->AddText(
                segu,
                22.0f,
                ImVec2(97.0f + windowPosition.x, 547.0f + windowPosition.y),
                ImColor(0.60f, 0.60f, 0.60f, 0.70f),
                "Reaperz\nEnhanced");

            ImGui::SetCursorPos(ImVec2(203.0f, 88.0f - size_child));
            switch (tabs)
            {
            case 0:
                ImGui::BeginChild("Self", ImVec2(339.0f, 253.0f), true);
                ImGui::Checkbox("God mode", &menu_settings.checkbox0);
                ImGui::Checkbox("Never wanted", &menu_settings.checkbox1);
                ImGui::SliderInt("Health", &menu_settings.sliderInteger, 1, 400);
                ImGui::SliderFloat("Run multiplier", &menu_settings.sliderFloat, 0.0f, 5.0f, "%.2f");
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(555.0f, 88.0f - size_child));
                ImGui::BeginChild("Theme", ImVec2(339.0f, 210.0f), true);
                ImGui::ColorEdit4(
                    "Background Color",
                    color_edit4,
                    ImGuiColorEditFlags_NoSidePreview |
                    ImGuiColorEditFlags_AlphaBar |
                    ImGuiColorEditFlags_NoInputs);
                ImGui::ColorEdit4(
                    "Widget Color",
                    accent_color,
                    ImGuiColorEditFlags_NoSidePreview |
                    ImGuiColorEditFlags_AlphaBar |
                    ImGuiColorEditFlags_NoInputs);
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(203.0f, 353.0f - size_child));
                ImGui::BeginChild("Quick Actions", ImVec2(339.0f, 258.0f), true);
                static const char* modes[] = { "Always", "Toggle" };
                ImGui::Combo("Feature mode", &menu_settings.aimbotMode, modes, IM_ARRAYSIZE(modes), 5);
                ImGui::InputText("Label", menu_settings.inputText.data(), menu_settings.inputText.size());
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(555.0f, 313.0f - size_child));
                ImGui::BeginChild("Keybinds", ImVec2(339.0f, 298.0f), true);
                ImGui::Keybind("Menu action", &menu_settings.aimbotKey, &menu_settings.aimbotKeyMode);
                ImGui::EndChild();
                break;

            case 1:
                DrawPlaceholderPage("Weapons", size_child);
                break;
            case 2:
                DrawPlaceholderPage("Teleport", size_child);
                break;
            case 3:
                DrawPlaceholderPage("Visuals", size_child);
                break;
            case 4:
                DrawPlaceholderPage("Misc", size_child);
                break;
            case 5:
                DrawPlaceholderPage("Players", size_child);
                break;
            case 6:
                DrawPlaceholderPage("Recovery", size_child);
                break;

            case 7:
                vehicle_menu.Render(size_child);
                break;

            case 8:
            {
                LuaManager& lua = LuaManager::Instance();
                auto& moduleManager = lua.GetModuleManager();
                auto& modules = moduleManager.GetModules();
                static int selected_script = 0;
                static char command_buffer[128] = "";

                if (!modules.empty() && selected_script >= moduleManager.GetCount())
                    selected_script = moduleManager.GetCount() - 1;

                ImGui::BeginChild("Scripts", ImVec2(339.0f, 253.0f), true);
                ImGui::TextUnformatted("Available Scripts");
                ImGui::Separator();
                ImGui::Spacing();
                if (modules.empty())
                {
                    ImGui::TextDisabled("No scripts found in scripts/");
                }
                else
                {
                    for (int index = 0; index < moduleManager.GetCount(); ++index)
                    {
                        ImGui::PushID(index);
                        bool enabled = modules[index].IsEnabled();
                        if (ImGui::Checkbox("##enabled", &enabled))
                            const_cast<LuaModule&>(modules[index]).SetEnabled(enabled);
                        ImGui::SameLine();
                        if (ImGui::Selectable(
                                modules[index].GetName().c_str(),
                                selected_script == index))
                        {
                            selected_script = index;
                        }
                        ImGui::PopID();
                    }
                }
                ImGui::Spacing();
                if (ImGui::Button("Load Script", ImVec2(150.0f, 28.0f)))
                    lua.RefreshScripts();
                ImGui::SameLine();
                if (ImGui::Button("Reload All", ImVec2(150.0f, 28.0f)))
                    moduleManager.ReloadAll(lua.GetState().lua_state());
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(555.0f, 88.0f - size_child));
                ImGui::BeginChild("ScriptInfo", ImVec2(339.0f, 210.0f), true);
                ImGui::TextUnformatted("Script Info");
                ImGui::Separator();
                ImGui::Spacing();
                if (!modules.empty() && selected_script < moduleManager.GetCount())
                {
                    const LuaModule& selected = modules[selected_script];
                    ImGui::Text("Name: %s", selected.GetName().c_str());
                    ImGui::Text("Author: %s", selected.GetAuthor().c_str());
                    ImGui::Text("Version: %s", selected.GetVersion().c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("Execute", ImVec2(150.0f, 28.0f)))
                        const_cast<LuaModule&>(selected).Execute(lua.GetState().lua_state());
                    ImGui::SameLine();
                    if (ImGui::Button("Stop", ImVec2(150.0f, 28.0f)))
                        const_cast<LuaModule&>(selected).Unload();
                }
                else
                {
                    ImGui::TextDisabled("No script selected");
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(203.0f, 353.0f - size_child));
                ImGui::BeginChild("ScriptOutput", ImVec2(339.0f, 258.0f), true);
                ImGui::TextUnformatted("Script Output");
                ImGui::Separator();
                std::string& output = const_cast<std::string&>(lua.GetOutputBuffer());
                ImGui::InputTextMultiline(
                    "##log",
                    output.data(),
                    output.size() + 1,
                    ImVec2(-1.0f, 195.0f),
                    ImGuiInputTextFlags_ReadOnly);
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputText(
                        "##command",
                        command_buffer,
                        sizeof(command_buffer),
                        ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    std::string result = lua.GetCommands().ExecuteCommand(
                        command_buffer, lua.GetState().lua_state());
                    if (result == "__CLEAR__")
                        lua.ClearOutput();
                    else
                        lua.AppendOutput("> " + std::string(command_buffer) + "\n" + result + "\n");
                    command_buffer[0] = '\0';
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(555.0f, 313.0f - size_child));
                ImGui::BeginChild("LuaPanel", ImVec2(339.0f, 298.0f), true);
                ImGui::TextUnformatted("Lua UI");
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::Button("Initialize Lua", ImVec2(150.0f, 28.0f)) &&
                    !lua.IsInitialized())
                {
                    lua.Initialize(scripts_path);
                }
                ImGui::SameLine();
                if (ImGui::Button("Shutdown Lua", ImVec2(150.0f, 28.0f)))
                    lua.Shutdown();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if (lua.IsInitialized())
                    lua.RenderCallbacks();
                else
                    ImGui::TextDisabled("Initialize Lua, then execute a script.");
                ImGui::EndChild();
                break;
            }

            case 9:
                ImGui::BeginChild("Configuration", ImVec2(691.0f, 523.0f), true);
                ImGui::TextUnformatted("Configuration");
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::Button("Save Settings", ImVec2(150.0f, 32.0f)))
                {
                    std::string error;
                    if (ConfigManager::Save(
                            settings_path, menu_settings, color_edit4, accent_color, error))
                    {
                        config_status = "Saved setting.json beside the executable.";
                    }
                    else
                    {
                        config_status = "Save failed: " + error;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Load Settings", ImVec2(150.0f, 32.0f)))
                {
                    std::string error;
                    if (ConfigManager::Load(
                            settings_path, menu_settings, color_edit4, accent_color, error))
                    {
                        config_status = "Loaded setting.json from beside the executable.";
                    }
                    else
                    {
                        config_status = "Load failed: " + error;
                    }
                }
                ImGui::Spacing();
                ImGui::TextDisabled("File: %s", settings_path.c_str());
                ImGui::Spacing();
                ImGui::TextWrapped("%s", config_status.c_str());
                ImGui::EndChild();
                break;
            }
        }
        ImGui::End();

        ImGui::Render();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        const D3DCOLOR clearColor = D3DCOLOR_RGBA(
            static_cast<int>(clear_color.x * clear_color.w * 255.0f),
            static_cast<int>(clear_color.y * clear_color.w * 255.0f),
            static_cast<int>(clear_color.z * clear_color.w * 255.0f),
            static_cast<int>(clear_color.w * 255.0f));

        g_pd3dDevice->Clear(
            0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clearColor, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        const HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST &&
            g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        {
            ResetDevice();
        }
    }

    vehicle_menu.Shutdown();
    LuaManager::Instance().Shutdown();

    if (scene != nullptr)
    {
        scene->Release();
        scene = nullptr;
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (g_pD3D == nullptr)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    RECT clientRect = {};
    if (::GetClientRect(hWnd, &clientRect))
    {
        g_d3dpp.BackBufferWidth = clientRect.right - clientRect.left;
        g_d3dpp.BackBufferHeight = clientRect.bottom - clientRect.top;
    }
    else
    {
        g_d3dpp.BackBufferWidth = kMenuWidth;
        g_d3dpp.BackBufferHeight = kMenuHeight;
    }

    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    return g_pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &g_d3dpp,
        &g_pd3dDevice) >= 0;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice != nullptr)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }

    if (g_pD3D != nullptr)
    {
        g_pD3D->Release();
        g_pD3D = nullptr;
    }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    const HRESULT result = g_pd3dDevice->Reset(&g_d3dpp);
    if (result == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
