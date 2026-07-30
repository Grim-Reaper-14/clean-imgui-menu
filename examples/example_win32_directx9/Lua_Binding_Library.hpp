#pragma once

struct lua_State;

// Registers all C-side functions into a Lua state so that scripts can call
// them.  Add new bindings in the corresponding .cpp file.
class LuaBindingLibrary
{
public:
    // Register every binding in the given state.
    static void Register(lua_State* L);

    // Remove every binding (sets globals to nil).
    static void Unregister(lua_State* L);

private:
    // ---- individual binding groups ----

    // console.print(msg)  – appends to the shared output buffer
    static void RegisterConsoleBindings(lua_State* L);

    // menu.get_accent_color() -> r,g,b,a
    // menu.set_accent_color(r,g,b,a)
    static void RegisterMenuBindings(lua_State* L);

    // utils.get_version() -> string
    // utils.get_tick_count() -> number
    static void RegisterUtilBindings(lua_State* L);
};
