#include "Lua_Binding_Library.hpp"
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

// Accent color shared with main.cpp
extern float accent_color[4];

// ---------------------------------------------------------------------------
// console.print(msg)
// ---------------------------------------------------------------------------
static int l_console_print(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    LuaManager::Instance().AppendOutput(std::string(msg) + "\n");
    return 0;
}

// console.clear()
static int l_console_clear(lua_State* L)
{
    (void)L;
    LuaManager::Instance().ClearOutput();
    return 0;
}

void LuaBindingLibrary::RegisterConsoleBindings(lua_State* L)
{
    luaL_Reg console_lib[] = {
        { "print", l_console_print },
        { "clear", l_console_clear },
        { nullptr, nullptr }
    };
    luaL_newlib(L, console_lib);
    lua_setglobal(L, "console");
}

// ---------------------------------------------------------------------------
// menu.get_accent_color() -> r, g, b, a
// menu.set_accent_color(r, g, b, a)
// ---------------------------------------------------------------------------
static int l_menu_get_accent_color(lua_State* L)
{
    lua_pushnumber(L, accent_color[0]);
    lua_pushnumber(L, accent_color[1]);
    lua_pushnumber(L, accent_color[2]);
    lua_pushnumber(L, accent_color[3]);
    return 4;
}

static int l_menu_set_accent_color(lua_State* L)
{
    accent_color[0] = (float)luaL_checknumber(L, 1);
    accent_color[1] = (float)luaL_checknumber(L, 2);
    accent_color[2] = (float)luaL_checknumber(L, 3);
    accent_color[3] = (float)luaL_optnumber(L, 4, 1.0);
    return 0;
}

void LuaBindingLibrary::RegisterMenuBindings(lua_State* L)
{
    luaL_Reg menu_lib[] = {
        { "get_accent_color", l_menu_get_accent_color },
        { "set_accent_color", l_menu_set_accent_color },
        { nullptr, nullptr }
    };
    luaL_newlib(L, menu_lib);
    lua_setglobal(L, "menu");
}

// ---------------------------------------------------------------------------
// utils.get_version()   -> string
// utils.get_tick_count() -> integer (milliseconds)
// ---------------------------------------------------------------------------
static int l_utils_get_version(lua_State* L)
{
    lua_pushstring(L, "Evicted 1.0");
    return 1;
}

static int l_utils_get_tick_count(lua_State* L)
{
#if defined(_WIN32)
    extern unsigned long __stdcall GetTickCount(void);
    lua_pushinteger(L, (lua_Integer)GetTickCount());
#else
    lua_pushinteger(L, 0);
#endif
    return 1;
}

void LuaBindingLibrary::RegisterUtilBindings(lua_State* L)
{
    luaL_Reg utils_lib[] = {
        { "get_version",    l_utils_get_version    },
        { "get_tick_count", l_utils_get_tick_count },
        { nullptr, nullptr }
    };
    luaL_newlib(L, utils_lib);
    lua_setglobal(L, "utils");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void LuaBindingLibrary::Register(lua_State* L)
{
    if (!L) return;
    luaL_openlibs(L);
    RegisterConsoleBindings(L);
    RegisterMenuBindings(L);
    RegisterUtilBindings(L);
}

void LuaBindingLibrary::Unregister(lua_State* L)
{
    if (!L) return;
    lua_pushnil(L); lua_setglobal(L, "console");
    lua_pushnil(L); lua_setglobal(L, "menu");
    lua_pushnil(L); lua_setglobal(L, "utils");
}
