#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")
#include <dxsdk-d3dx/d3dx9.h>
#pragma comment(lib,"d3dx9.lib")

#include <tchar.h>
#include <filesystem>
#include <string>
#include <system_error>

#include "background_pic.h"
#include "blur.hpp"
#include "config/ConfigManager.hpp"

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
        std::wstring module_path(MAX_PATH, L'\0');

        for (;;)
        {
            const DWORD length = ::GetModuleFileNameW(
                nullptr, module_path.data(),
                static_cast<DWORD>(module_path.size()));

            if (length == 0)
                return std::filesystem::current_path();

            if (length < module_path.size())
            {
                module_path.resize(length);
                return std::filesystem::path(module_path).parent_path();
            }

            module_path.resize(module_path.size() * 2);
        }
    }
}

float color_edit4[4] = { 1.00f, 1.00f, 1.00f, 1.000f };

float accent_color[4] = { 0.745f, 0.151f, 0.151f, 1.000f };

static MenuSettings menu_settings;
static const std::filesystem::path executable_directory =
    GetExecutableDirectory();
static const std::string settings_path =
    (executable_directory / L"setting.json").string();
static const std::string scripts_path =
    (executable_directory / L"scripts").string();

static int select_count = 0;

bool active = false;

float size_child = 0;

bool menu = false;
ImFont* ico = nullptr;
ImFont* ico_combo = nullptr;
ImFont* ico_button = nullptr;
ImFont* ico_grande = nullptr;
ImFont* segu = nullptr;
ImFont* default_segu = nullptr;
ImFont* bold_segu = nullptr;
LPDIRECT3D9              g_pD3D = NULL;
LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
IDirect3DTexture9* scene = nullptr;

int tab_count = 0, tabs = 0;

int main(int, char**)
{

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    RECT window_rect = { 0, 0, kMenuWidth, kMenuHeight };
    ::AdjustWindowRectEx(&window_rect, kWindowStyle, FALSE, 0);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"Dear ImGui DirectX9 Example", kWindowStyle,
        200, 200, window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top, NULL, NULL, wc.hInstance, NULL);

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
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGuiContext& g = *GImGui;
    io.Fonts->AddFontFromMemoryTTF(&seguoe, sizeof seguoe, 22, NULL, io.Fonts->GetGlyphRangesCyrillic());

    default_segu = io.Fonts->AddFontFromMemoryTTF(&seguoe, sizeof seguoe, 22, NULL, io.Fonts->GetGlyphRangesCyrillic());

    segu = io.Fonts->AddFontFromMemoryTTF(&seguoe, sizeof seguoe, 40, NULL, io.Fonts->GetGlyphRangesCyrillic());

    bold_segu = io.Fonts->AddFontFromMemoryTTF(&bold_segue, sizeof bold_segue, 40, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ico = io.Fonts->AddFontFromMemoryTTF(&icon, sizeof icon, 24, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ico_combo = io.Fonts->AddFontFromMemoryTTF(&icon, sizeof icon, 19, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ico_button = io.Fonts->AddFontFromMemoryTTF(&icon, sizeof icon, 25, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ico_grande = io.Fonts->AddFontFromMemoryTTF(&icon, sizeof icon, 40, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    std::string config_status =
        "No setting.json found beside the executable; using defaults.";
    std::error_code settings_error;
    if (std::filesystem::exists(settings_path, settings_error))
    {
        std::string error;
        if (ConfigManager::Load(settings_path, menu_settings,
                color_edit4, accent_color, error))
        {
            config_status =
                "Loaded setting.json automatically from beside the executable.";
        }
        else
        {
            config_status = "Automatic load failed: " + error;
            const std::string debug_message = config_status + "\n";
            ::OutputDebugStringA(debug_message.c_str());
        }
    }
    else if (settings_error)
    {
        config_status =
            "Automatic load failed: " + settings_error.message();
        const std::string debug_message = config_status + "\n";
        ::OutputDebugStringA(debug_message.c_str());
    }

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done = false;
    while (!done)
    {

        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        DWORD window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(static_cast<float>(kMenuWidth),
                       static_cast<float>(kMenuHeight)),
                ImGuiCond_Always);

            ImGui::Begin("Hola!", &menu, window_flags);
            {
                ImVec2 P1, P2;
                ImDrawList* pDrawList;
                const auto& p = ImGui::GetWindowPos();
                const auto& pWindowDrawList = ImGui::GetWindowDrawList();
                const auto& pBackgroundDrawList = ImGui::GetBackgroundDrawList();
                const auto& pForegroundDrawList = ImGui::GetForegroundDrawList();

                DrawBackgroundBlur(pWindowDrawList, g_pd3dDevice);


                if (scene == nullptr) D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &background, sizeof(background), 1920, 1080, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &scene);

                ImGui::GetBackgroundDrawList()->AddImage(scene, ImVec2(0, 0), ImVec2(1920, 1080), ImVec2(0, 0), ImVec2(1, 1), ImColor(color_edit4[0], color_edit4[1], color_edit4[2], color_edit4[3]));

                pBackgroundDrawList->AddRectFilled(ImVec2(0.000f + p.x, 0.000f + p.y), ImVec2(kMenuWidth + p.x, kMenuHeight + p.y), ImColor(9, 9, 9, 180), 10); // Background

                pWindowDrawList->AddRectFilled(ImVec2(189.000f + p.x, 75.000f + p.y), ImVec2(903 + p.x, 76 + p.y), ImColor(25, 25, 25, 180), 10); // bar line

                ImGui::SetCursorPos(ImVec2(800, 21));

                if (ImGui::OptButton("L", ImVec2(30, 30), false));

                ImGui::SameLine(840);

                if(ImGui::OptButton("B", ImVec2(30, 30), true));

                pWindowDrawList->AddRectFilled(ImVec2(0.000f + p.x, 0.000f + p.y), ImVec2(190 + p.x, kMenuHeight + p.y), ImGui::GetColorU32(ImGuiCol_ChildBg), 10, ImDrawFlags_RoundCornersLeft); // bar line

                    const int vtx_idx_1 = pWindowDrawList->VtxBuffer.Size;

                    pWindowDrawList->AddText(ico_grande, 40.f, ImVec2(20.000f + p.x, 20.000f + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.70f), "U");

                    pWindowDrawList->AddText(bold_segu, 40.f, ImVec2(70.000f + p.x, 15.000f + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.70f), "Evicted");

                    pWindowDrawList->AddRectFilled(ImVec2(70.000f + p.x, 51.000f + p.y), ImVec2(163 + p.x, 52 + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.70f), 10); // bar line

                    const int vtx_idx_2 = pWindowDrawList->VtxBuffer.Size;

                    ImGui::ShadeVertsLinearColorGradientKeepAlpha(pWindowDrawList, vtx_idx_1, vtx_idx_2, ImVec2(0 + p.x, 0 + p.y), ImVec2(200 + p.x, 20 + p.y), ImColor(0.25f, 0.25f, 0.25f, 0.50f), ImColor(0.60f, 0.60f, 0.60f, 1.00f));

                    ImGui::SetCursorPosY(80);

                    if (ImGui::TabButton("P", "LegitBot", ImVec2(190, 40)) && tab_count != 0) {
                        tab_count = 0;
                        active = true;
                    }
                    if (ImGui::TabButton("N", "RageBot", ImVec2(190, 40)) && tab_count != 1) {
                        tab_count = 1;
                        active = true;
                    }

                    if (ImGui::TabButton("Q", "AntiAim", ImVec2(190, 40)) && tab_count != 2) {
                        tab_count = 2;
                        active = true;
                    }

                    if (ImGui::TabButton("I", "Visuals", ImVec2(190, 40)) && tab_count != 3) {
                        tab_count = 3;
                        active = true;
                    }

                    if (ImGui::TabButton("O", "Misc", ImVec2(190, 40)) && tab_count != 4) {
                        tab_count = 4;
                        active = true;
                    }

                    if (ImGui::TabButton("R", "PlayerList", ImVec2(190, 40)) && tab_count != 5) {
                        tab_count = 5;
                        active = true;
                    }

                    if (ImGui::TabButton("T", "Skins", ImVec2(190, 40)) && tab_count != 6) {
                        tab_count = 6;
                        active = true;
                    }

                    if (ImGui::TabButton("J", "Lua", ImVec2(190, 40)) && tab_count != 7) {
                        tab_count = 7;
                        active = true;
                    }

                    if (ImGui::TabButton("S", "Config", ImVec2(190, 40)) && tab_count != 8) {
                        tab_count = 8;
                        active = true;
                    }

                if (active) {
                    if (size_child <= 10) size_child += 1 / ImGui::GetIO().Framerate * 60.f;
                    else { active = false; tabs = tab_count; };
                }
                else {
                    if (size_child >= 0) size_child -= 1 / ImGui::GetIO().Framerate * 60.f;
                }

                pWindowDrawList->AddCircleFilled(ImVec2(57.000f + p.x, 570.000f + p.y), 25.000f, ImColor(10, 9, 10, 255), 30);

                pWindowDrawList->AddCircle(ImVec2(57.000f + p.x, 570.000f + p.y), 27.000f, ImColor(20, 19, 20, 255), 30, 4.000f);

                pWindowDrawList->AddText(segu, 40.f, ImVec2(51.000f + p.x, 548.000f + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.50f), "?");



                const int vtx_idx_3 = pWindowDrawList->VtxBuffer.Size;

                pWindowDrawList->AddText(segu, 22.f, ImVec2(97.000f + p.x, 547.000f + p.y), ImColor(0.40f, 0.40f, 0.40f, 0.50f), "Benjy\nLifetime");

                const int vtx_idx_4 = pWindowDrawList->VtxBuffer.Size;

                ImGui::ShadeVertsLinearColorGradientKeepAlpha(pWindowDrawList, vtx_idx_3, vtx_idx_4, ImVec2(97.000f + p.x, 547.000f + p.y), ImVec2(200.000f + p.x, 567.000f + p.y), ImColor(0.35f, 0.35f, 0.35f, 0.50f), ImColor(0.90f, 0.90f, 0.90f, 1.00f));



                ImGui::SetCursorPos(ImVec2(203, 88 - size_child));

                switch (tabs) {

                case 0:

                    ImGui::BeginChild("Editor", ImVec2(339, 253), true); {

                        ImGui::Checkbox("Checkbox_0", &menu_settings.checkbox0);

                        ImGui::Checkbox("Checkbox_1", &menu_settings.checkbox1);

                        ImGui::SliderInt("Slider Intager",
                            &menu_settings.sliderInteger, 1, 400);

                        ImGui::SliderFloat("Slider Float",
                            &menu_settings.sliderFloat, 0.00f, 5.00f, "%.2f");

                    }ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(555, 88 - size_child));

                    ImGui::BeginChild("Background", ImVec2(339, 210), true); {

                        ImGui::ColorEdit4("Background Color", (float*)&color_edit4, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);

                        ImGui::ColorEdit4("Widget Color", (float*)&accent_color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);

                    }ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(203, 353 - size_child));

                    ImGui::BeginChild("Aimbot", ImVec2(339, 258), true); {

                        static const char* items[]{ "Always", "Toggle" };
                        ImGui::Combo("Aimbot Mode", &menu_settings.aimbotMode,
                            items, IM_ARRAYSIZE(items), 5);

                        ImGui::InputText("InputText", menu_settings.inputText.data(),
                            menu_settings.inputText.size());

                    }ImGui::EndChild();


                    ImGui::SetCursorPos(ImVec2(555, 313 - size_child));

                    ImGui::BeginChild("Keybinds", ImVec2(339, 298), true); {

                        ImGui::Keybind("Aimbot Keybind", &menu_settings.aimbotKey,
                            &menu_settings.aimbotKeyMode);

                    }ImGui::EndChild();

                    break;

                case 7:
                {
                    LuaManager& lua      = LuaManager::Instance();
                    auto& modMgr         = lua.GetModuleManager();
                    auto& modules        = modMgr.GetModules();

                    static int  selected_script = 0;
                    static char cmd_buf[128]    = "";

                    // Clamp selection to valid range
                    if (!modules.empty() && selected_script >= modMgr.GetCount())
                        selected_script = modMgr.GetCount() - 1;

                    // ---- left-top: script list ----
                    ImGui::BeginChild("Scripts", ImVec2(339, 253), true); {
                        ImGui::Text("Available Scripts");
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (modules.empty())
                        {
                            ImGui::TextDisabled("No scripts found in scripts/");
                        }
                        else
                        {
                            for (int i = 0; i < modMgr.GetCount(); i++)
                            {
                                ImGui::PushID(i);
                                bool en = modules[i].IsEnabled();
                                if (ImGui::Checkbox("##en", &en))
                                    const_cast<LuaModule&>(modules[i]).SetEnabled(en);
                                ImGui::SameLine();
                                if (ImGui::Selectable(modules[i].GetName().c_str(), selected_script == i))
                                    selected_script = i;
                                ImGui::PopID();
                            }
                        }

                        ImGui::Spacing();
                        if (ImGui::Button("Load Script", ImVec2(150, 28)))
                            lua.RefreshScripts();
                        ImGui::SameLine();
                        if (ImGui::Button("Reload All", ImVec2(150, 28)))
                            modMgr.ReloadAll(lua.GetState().lua_state());
                    } ImGui::EndChild();

                    // ---- right-top: script info / execution ----
                    ImGui::SetCursorPos(ImVec2(555, 88 - size_child));
                    ImGui::BeginChild("ScriptInfo", ImVec2(339, 210), true); {
                        static const char* status_labels[] = { "Stopped", "Running", "Error" };
                        static const ImVec4 status_colors[] = {
                            ImVec4(0.55f, 0.55f, 0.55f, 1.0f),
                            ImVec4(0.20f, 0.80f, 0.20f, 1.0f),
                            ImVec4(0.80f, 0.20f, 0.20f, 1.0f)
                        };

                        ImGui::Text("Script Info");
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (!modules.empty() && selected_script < modMgr.GetCount())
                        {
                            const LuaModule& sel = modules[selected_script];
                            ImGui::Text("Name:    %s", sel.GetName().c_str());
                            ImGui::Text("Author:  %s", sel.GetAuthor().c_str());
                            ImGui::Text("Version: %s", sel.GetVersion().c_str());
                            ImGui::Spacing();
                            int si = static_cast<int>(sel.GetStatus());
                            ImGui::TextColored(status_colors[si], "Status:  %s", status_labels[si]);
                            ImGui::Spacing();
                            if (ImGui::Button("Execute", ImVec2(150, 28)))
                                const_cast<LuaModule&>(sel).Execute(lua.GetState().lua_state());
                            ImGui::SameLine();
                            if (ImGui::Button("Stop", ImVec2(150, 28)))
                                const_cast<LuaModule&>(sel).Unload();
                        }
                        else
                        {
                            ImGui::TextDisabled("No script selected");
                        }
                    } ImGui::EndChild();

                    // ---- left-bottom: script output ----
                    ImGui::SetCursorPos(ImVec2(203, 353 - size_child));
                    ImGui::BeginChild("ScriptOutput", ImVec2(339, 258), true); {
                        ImGui::Text("Script Output");
                        ImGui::Separator();

                        // Read-only log driven by LuaManager's output buffer
                        std::string& outBuf = const_cast<std::string&>(lua.GetOutputBuffer());
                        ImGui::InputTextMultiline("##log",
                            outBuf.data(), outBuf.size() + 1,
                            ImVec2(-1, 195), ImGuiInputTextFlags_ReadOnly);

                        // Command input
                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::InputText("##cmd", cmd_buf, sizeof(cmd_buf),
                                             ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            std::string result = lua.GetCommands().ExecuteCommand(cmd_buf, lua.GetState().lua_state());
                            if (result == "__CLEAR__")
                                lua.ClearOutput();
                            else
                                lua.AppendOutput("> " + std::string(cmd_buf) + "\n" + result + "\n");
                            cmd_buf[0] = '\0';
                        }
                    } ImGui::EndChild();

                    // ---- right-bottom: inline Lua UI ----
                    ImGui::SetCursorPos(ImVec2(555, 313 - size_child));
                    ImGui::BeginChild("LuaPanel", ImVec2(339, 298), true); {
                        ImGui::Text("Lua UI");
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (ImGui::Button("Initialize Lua", ImVec2(150, 28)))
                        {
                            if (!lua.IsInitialized())
                                lua.Initialize(scripts_path);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Shutdown Lua", ImVec2(150, 28)))
                            lua.Shutdown();

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (lua.IsInitialized())
                            lua.RenderCallbacks();
                        else
                            ImGui::TextDisabled("Initialize Lua, then execute a script.");
                    } ImGui::EndChild();

                    break;
                }

                case 8:
                {
                    ImGui::BeginChild("Configuration", ImVec2(339, 253), true); {
                        ImGui::Text("Configuration");
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (ImGui::Button("Save Settings", ImVec2(150, 32)))
                        {
                            std::string error;
                            if (ConfigManager::Save(settings_path, menu_settings,
                                    color_edit4, accent_color, error))
                            {
                                config_status =
                                    "Saved setting.json beside the executable.";
                            }
                            else
                            {
                                config_status = "Save failed: " + error;
                            }
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Load Settings", ImVec2(150, 32)))
                        {
                            std::string error;
                            if (ConfigManager::Load(settings_path, menu_settings,
                                    color_edit4, accent_color, error))
                            {
                                config_status =
                                    "Loaded setting.json from beside the executable.";
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

                    }ImGui::EndChild();

                    break;
                }

                }
            }
            ImGui::End();
        }
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x*clear_color.w*255.0f), (int)(clear_color.y*clear_color.w*255.0f), (int)(clear_color.z*clear_color.w*255.0f), (int)(clear_color.w*255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    LuaManager::Instance().Shutdown();
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
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    RECT client_rect = {};
    if (::GetClientRect(hWnd, &client_rect))
    {
        g_d3dpp.BackBufferWidth = client_rect.right - client_rect.left;
        g_d3dpp.BackBufferHeight = client_rect.bottom - client_rect.top;
    }
    else
    {
        g_d3dpp.BackBufferWidth = kMenuWidth;
        g_d3dpp.BackBufferHeight = kMenuHeight;
    }

    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
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
