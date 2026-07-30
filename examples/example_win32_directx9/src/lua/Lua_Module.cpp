#include "Lua_Module.hpp"

// Lua 5.4 C API.  Add the Lua 5.4 SDK include path to the project properties
// (e.g. C:\Lua54\include) and lua54.lib to the linker additional dependencies.
#ifdef LUA_VERSION_NUM
// Headers already included transitively
#else
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

#include <algorithm>
#include <filesystem>

// ---------------------------------------------------------------------------
LuaModule::LuaModule(const std::string& name, const std::string& filePath)
    : m_name(name), m_filePath(filePath)
{
}

LuaModule::~LuaModule()
{
    // Ownership of the lua_State belongs to LuaManager; we only unmark here.
    m_loaded = false;
}

// ---------------------------------------------------------------------------
bool LuaModule::Load(lua_State* L)
{
    if (!L)
    {
        m_lastError = "lua_State is null";
        m_status    = LuaScriptStatus::Error;
        return false;
    }

    int result = luaL_loadfile(L, m_filePath.c_str());
    if (result != LUA_OK)
    {
        m_lastError = lua_tostring(L, -1);
        lua_pop(L, 1);
        m_status = LuaScriptStatus::Error;
        m_loaded = false;
        return false;
    }

    // Keep the compiled chunk on the stack (used by Execute).
    // We pop it immediately; Execute will re-load from file each call.
    lua_pop(L, 1);

    m_loaded    = true;
    m_lastError = "";
    m_status    = LuaScriptStatus::Stopped;
    return true;
}

// ---------------------------------------------------------------------------
bool LuaModule::Execute(lua_State* L)
{
    if (!L)
    {
        m_lastError = "lua_State is null";
        m_status    = LuaScriptStatus::Error;
        return false;
    }

    if (!m_enabled)
        return true;

    // Load+execute in one step via luaL_dofile so the chunk runs fresh.
    m_status = LuaScriptStatus::Running;
    int result = luaL_dofile(L, m_filePath.c_str());
    if (result != LUA_OK)
    {
        m_lastError = lua_tostring(L, -1);
        lua_pop(L, 1);
        m_status = LuaScriptStatus::Error;
        return false;
    }

    m_lastError = "";
    m_status    = LuaScriptStatus::Running;
    return true;
}

// ---------------------------------------------------------------------------
bool LuaModule::Reload(lua_State* L)
{
    Unload();
    if (!Load(L))
        return false;
    return Execute(L);
}

// ---------------------------------------------------------------------------
void LuaModule::Unload()
{
    m_loaded = false;
    m_status = LuaScriptStatus::Stopped;
}
