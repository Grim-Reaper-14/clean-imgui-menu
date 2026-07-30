#include "Lua_Manager.hpp"

#ifdef LUA_VERSION_NUM
// already included
#else
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

// ---------------------------------------------------------------------------
LuaManager& LuaManager::Instance()
{
    static LuaManager instance;
    return instance;
}

// ---------------------------------------------------------------------------
bool LuaManager::Initialize(const std::string& scriptsDirectory)
{
    if (m_L)
        Shutdown(); // re-initialize guard

    m_L = luaL_newstate();
    if (!m_L)
    {
        AppendOutput("[ERROR] Failed to create lua_State\n");
        return false;
    }

    // Register all C bindings
    LuaBindingLibrary::Register(m_L);

    // Register built-in commands
    m_commands.RegisterBuiltins();

    // Set up the scripts directory and scan for files
    m_scriptsManager.SetDirectory(scriptsDirectory);
    m_scriptsManager.EnsureDirectoryExists();

    // Add discovered scripts to the module manager
    for (const auto& path : m_scriptsManager.GetScriptFiles())
        m_moduleManager.AddModule(path);

    // Load (compile) all enabled modules
    m_moduleManager.LoadAll(m_L);

    AppendOutput("[INFO] Lua system initialized\n");
    AppendOutput("[INFO] Scripts directory: " + scriptsDirectory + "\n");
    AppendOutput("[INFO] Scripts found: " +
                 std::to_string(m_scriptsManager.GetScriptCount()) + "\n");
    return true;
}

// ---------------------------------------------------------------------------
void LuaManager::Update()
{
    // Intentionally lightweight: execute only enabled, already-loaded modules.
    // Call this each frame (or throttle via a timer) from the main loop.
    m_moduleManager.ExecuteAll(m_L);
}

// ---------------------------------------------------------------------------
void LuaManager::Shutdown()
{
    if (!m_L)
        return;

    m_moduleManager.Clear();
    LuaBindingLibrary::Unregister(m_L);
    lua_close(m_L);
    m_L = nullptr;
}

// ---------------------------------------------------------------------------
void LuaManager::AppendOutput(const std::string& text)
{
    m_outputBuffer += text;

    // Keep the buffer from growing unbounded
    constexpr size_t kMaxBuffer = 4096;
    if (m_outputBuffer.size() > kMaxBuffer)
        m_outputBuffer.erase(0, m_outputBuffer.size() - kMaxBuffer);
}

// ---------------------------------------------------------------------------
void LuaManager::ClearOutput()
{
    m_outputBuffer.clear();
}

// ---------------------------------------------------------------------------
void LuaManager::RefreshScripts()
{
    m_moduleManager.Clear();
    m_scriptsManager.ScanDirectory();

    for (const auto& path : m_scriptsManager.GetScriptFiles())
        m_moduleManager.AddModule(path);

    if (m_L)
        m_moduleManager.LoadAll(m_L);

    AppendOutput("[INFO] Scripts refreshed. Found: " +
                 std::to_string(m_scriptsManager.GetScriptCount()) + "\n");
}
