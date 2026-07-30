#pragma once
#include "Lua_Module_Manager.hpp"
#include "Lua_Scripts_Manager.hpp"
#include "Lua_Commands.hpp"
#include "Lua_Binding_Library.hpp"  // pulls in sol.hpp
#include <string>

// Top-level singleton that owns the sol::state and co-ordinates all Lua
// subsystems.  Call LuaManager::Instance() to access it.
class LuaManager
{
public:
    static LuaManager& Instance();

    // Open a new sol::state, register bindings, and scan the scripts directory.
    // Must be called before any other method.
    bool Initialize(const std::string& scriptsDirectory = "scripts");

    // Execute every enabled, loaded script once (called each frame if desired).
    void Update();

    // Close the Lua state and release all resources.
    void Shutdown();

    // True after Initialize() and before Shutdown().
    bool IsInitialized() const { return m_initialized; }

    // ---- subsystem accessors ----
    sol::state&         GetState()          { return m_lua; }
    LuaModuleManager&   GetModuleManager()  { return m_moduleManager; }
    LuaScriptsManager&  GetScriptsManager() { return m_scriptsManager; }
    LuaCommands&        GetCommands()       { return m_commands; }

    // Shared output buffer written to by the console.print() binding.
    // The UI reads this to populate the Script Output child window.
    const std::string& GetOutputBuffer() const { return m_outputBuffer; }
    void               AppendOutput(const std::string& text);
    void               ClearOutput();

    // Rescan the scripts directory and refresh loaded modules.
    void RefreshScripts();

private:
    LuaManager()  = default;
    ~LuaManager() = default;
    LuaManager(const LuaManager&)            = delete;
    LuaManager& operator=(const LuaManager&) = delete;

    sol::state         m_lua;
    bool               m_initialized = false;
    LuaModuleManager   m_moduleManager;
    LuaScriptsManager  m_scriptsManager;
    LuaCommands        m_commands;
    std::string        m_outputBuffer;
};
