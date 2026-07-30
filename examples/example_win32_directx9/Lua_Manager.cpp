#include "Lua_Manager.hpp"

// ---------------------------------------------------------------------------
LuaManager& LuaManager::Instance()
{
    static LuaManager instance;
    return instance;
}

// ---------------------------------------------------------------------------
bool LuaManager::Initialize(const std::string& scriptsDirectory)
{
    if (m_initialized)
        Shutdown(); // re-initialize guard

    // sol::state default-constructs a fresh lua_State with no libraries open.
    m_lua = sol::state{};

    // Register all bindings (also calls lua.open_libraries internally)
    LuaBindingLibrary::Register(m_lua);

    // Register built-in commands
    m_commands.RegisterBuiltins();

    // Set up the scripts directory and scan for files
    m_scriptsManager.SetDirectory(scriptsDirectory);
    m_scriptsManager.EnsureDirectoryExists();

    // Add discovered scripts to the module manager
    for (const auto& path : m_scriptsManager.GetScriptFiles())
        m_moduleManager.AddModule(path);

    // Load (compile) all enabled modules
    m_moduleManager.LoadAll(m_lua.lua_state());

    m_initialized = true;

    AppendOutput("[INFO] Lua (sol2) system initialized\n");
    AppendOutput("[INFO] Scripts directory: " + scriptsDirectory + "\n");
    AppendOutput("[INFO] Scripts found: " +
                 std::to_string(m_scriptsManager.GetScriptCount()) + "\n");
    return true;
}

// ---------------------------------------------------------------------------
void LuaManager::Update()
{
    // Intentionally lightweight: execute only enabled, already-loaded modules.
    m_moduleManager.ExecuteAll(m_lua.lua_state());
}

// ---------------------------------------------------------------------------
void LuaManager::Shutdown()
{
    if (!m_initialized)
        return;

    m_moduleManager.Clear();
    LuaBindingLibrary::Unregister(m_lua);

    // Destroy the sol::state (closes the underlying lua_State).
    m_lua = sol::state{};
    m_initialized = false;
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

    if (m_initialized)
        m_moduleManager.LoadAll(m_lua.lua_state());

    AppendOutput("[INFO] Scripts refreshed. Found: " +
                 std::to_string(m_scriptsManager.GetScriptCount()) + "\n");
}
