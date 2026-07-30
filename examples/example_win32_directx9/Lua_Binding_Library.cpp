#include "Lua_Binding_Library.hpp"
#include "Lua_Manager.hpp"

// Accent color shared with main.cpp
extern float accent_color[4];

// ---------------------------------------------------------------------------
// console
// ---------------------------------------------------------------------------
void LuaBindingLibrary::RegisterConsoleBindings(sol::state& lua)
{
    sol::table console = lua.create_named_table("console");

    console.set_function("print", [](const std::string& msg)
    {
        LuaManager::Instance().AppendOutput(msg + "\n");
    });

    console.set_function("clear", []()
    {
        LuaManager::Instance().ClearOutput();
    });
}

// ---------------------------------------------------------------------------
// menu
// ---------------------------------------------------------------------------
void LuaBindingLibrary::RegisterMenuBindings(sol::state& lua)
{
    sol::table menu = lua.create_named_table("menu");

    // Returns r, g, b, a as a table { r, g, b, a }
    menu.set_function("get_accent_color", []() -> sol::table
    {
        sol::state_view view(LuaManager::Instance().GetState().lua_state());
        sol::table t = view.create_table_with(
            "r", accent_color[0],
            "g", accent_color[1],
            "b", accent_color[2],
            "a", accent_color[3]);
        return t;
    });

    menu.set_function("set_accent_color",
        [](float r, float g, float b, sol::optional<float> a)
        {
            accent_color[0] = r;
            accent_color[1] = g;
            accent_color[2] = b;
            accent_color[3] = a.value_or(1.0f);
        });
}

// ---------------------------------------------------------------------------
// utils
// ---------------------------------------------------------------------------
void LuaBindingLibrary::RegisterUtilBindings(sol::state& lua)
{
    sol::table utils = lua.create_named_table("utils");

    utils.set_function("get_version", []() -> std::string
    {
        return "Evicted 1.0";
    });

    utils.set_function("get_tick_count", []() -> unsigned long
    {
#if defined(_WIN32)
        return ::GetTickCount();
#else
        return 0UL;
#endif
    });
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void LuaBindingLibrary::Register(sol::state& lua)
{
    lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::string,
        sol::lib::table,
        sol::lib::math,
        sol::lib::io,
        sol::lib::os);

    RegisterConsoleBindings(lua);
    RegisterMenuBindings(lua);
    RegisterUtilBindings(lua);
}

void LuaBindingLibrary::Unregister(sol::state& lua)
{
    lua["console"] = sol::nil;
    lua["menu"]    = sol::nil;
    lua["utils"]   = sol::nil;
}
