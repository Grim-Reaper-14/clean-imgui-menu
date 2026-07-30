#pragma once

// sol2 and Lua are supplied by the repository's vcpkg manifest.
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

// Registers all application-side functions into a sol::state so that scripts
// can call them.  Add new binding groups in the corresponding .cpp file.
class LuaBindingLibrary
{
public:
    // Register every binding in the given state.
    static void Register(sol::state& lua);

    // Remove every binding (sets globals to nil / sol::nil).
    static void Unregister(sol::state& lua);

private:
    // ---- individual binding groups ----

    // console.print(msg)  – appends to the shared output buffer
    // console.clear()     – clears the output buffer
    static void RegisterConsoleBindings(sol::state& lua);

    // menu.get_accent_color() -> r, g, b, a
    // menu.set_accent_color(r, g, b, a)
    static void RegisterMenuBindings(sol::state& lua);

    // utils.get_version()    -> string
    // utils.get_tick_count() -> integer (ms)
    static void RegisterUtilBindings(sol::state& lua);

    // filesystem.* provides file, directory, and path helpers.
    static void RegisterFileSystemBindings(sol::state& lua);

    // logger.* writes timestamped messages to the UI, debugger, and log file.
    static void RegisterLoggerBindings(sol::state& lua);
};
