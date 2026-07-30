#include "Lua_Binding_Library.hpp"
#include "FileSystemAPI.hpp"
#include "ImGuiAPI.hpp"
#include "LoggerAPI.hpp"
#include "Lua_Manager.hpp"

#include <Windows.h>
#include <stdexcept>

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
// filesystem
// ---------------------------------------------------------------------------
void LuaBindingLibrary::RegisterFileSystemBindings(sol::state& lua)
{
    sol::table filesystem = lua.create_named_table("filesystem");

    filesystem.set_function("exists", &FileSystemAPI::Exists);
    filesystem.set_function("is_file", &FileSystemAPI::IsFile);
    filesystem.set_function("is_directory", &FileSystemAPI::IsDirectory);
    filesystem.set_function("read_file", &FileSystemAPI::ReadFile);
    filesystem.set_function("write_file", &FileSystemAPI::WriteFile);
    filesystem.set_function("append_file", &FileSystemAPI::AppendFile);
    filesystem.set_function("create_directory", &FileSystemAPI::CreateDirectories);

    filesystem.set_function("remove",
        [](const std::string& path, sol::optional<bool> recursive)
        {
            return FileSystemAPI::Remove(path, recursive.value_or(false));
        });

    filesystem.set_function("copy",
        [](const std::string& source, const std::string& destination,
           sol::optional<bool> overwrite)
        {
            FileSystemAPI::Copy(source, destination, overwrite.value_or(true));
        });

    filesystem.set_function("move",
        [](const std::string& source, const std::string& destination,
           sol::optional<bool> overwrite)
        {
            FileSystemAPI::Move(source, destination, overwrite.value_or(true));
        });

    filesystem.set_function("list",
        [](sol::this_state state, const std::string& path,
           sol::optional<bool> recursive) -> sol::table
        {
            sol::state_view view(state);
            sol::table result = view.create_table();
            const auto entries =
                FileSystemAPI::ListDirectory(path, recursive.value_or(false));

            for (std::size_t index = 0; index < entries.size(); ++index)
                result[index + 1] = entries[index];

            return result;
        });

    filesystem.set_function("file_size", &FileSystemAPI::FileSize);
    filesystem.set_function("current_directory", &FileSystemAPI::CurrentDirectory);
    filesystem.set_function("absolute_path", &FileSystemAPI::AbsolutePath);
    filesystem.set_function("join", &FileSystemAPI::Join);
    filesystem.set_function("file_name", &FileSystemAPI::FileName);
    filesystem.set_function("extension", &FileSystemAPI::Extension);
    filesystem.set_function("parent_path", &FileSystemAPI::ParentPath);
}

// ---------------------------------------------------------------------------
// logger
// ---------------------------------------------------------------------------
void LuaBindingLibrary::RegisterLoggerBindings(sol::state& lua)
{
    sol::table logger = lua.create_named_table("logger");

    logger.set_function("trace",
        [](const std::string& message) { return LoggerAPI::Instance().Trace(message); });
    logger.set_function("debug",
        [](const std::string& message) { return LoggerAPI::Instance().Debug(message); });
    logger.set_function("info",
        [](const std::string& message) { return LoggerAPI::Instance().Info(message); });
    logger.set_function("warn",
        [](const std::string& message) { return LoggerAPI::Instance().Warning(message); });
    logger.set_function("warning",
        [](const std::string& message) { return LoggerAPI::Instance().Warning(message); });
    logger.set_function("error",
        [](const std::string& message) { return LoggerAPI::Instance().Error(message); });
    logger.set_function("critical",
        [](const std::string& message) { return LoggerAPI::Instance().Critical(message); });

    logger.set_function("log",
        [](const std::string& levelName, const std::string& message)
        {
            LogLevel level;
            if (!LoggerAPI::TryParseLevel(levelName, level))
                throw std::invalid_argument("Unknown log level: " + levelName);
            return LoggerAPI::Instance().Log(level, message);
        });

    logger.set_function("set_level",
        [](const std::string& levelName)
        {
            LogLevel level;
            if (!LoggerAPI::TryParseLevel(levelName, level))
                throw std::invalid_argument("Unknown log level: " + levelName);
            LoggerAPI::Instance().SetMinimumLevel(level);
        });

    logger.set_function("get_level", []() -> std::string
    {
        return LoggerAPI::LevelName(LoggerAPI::Instance().GetMinimumLevel());
    });

    logger.set_function("set_file",
        [](const std::string& path) { LoggerAPI::Instance().SetLogFile(path); });
    logger.set_function("get_file",
        []() { return LoggerAPI::Instance().GetLogFile(); });
    logger.set_function("enable_file",
        [](bool enabled) { LoggerAPI::Instance().EnableFileLogging(enabled); });
    logger.set_function("is_file_enabled",
        []() { return LoggerAPI::Instance().IsFileLoggingEnabled(); });
    logger.set_function("clear_file",
        []() { return LoggerAPI::Instance().ClearLogFile(); });
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
    RegisterFileSystemBindings(lua);
    RegisterLoggerBindings(lua);
    ImGuiAPI::Register(lua);
}

void LuaBindingLibrary::Unregister(sol::state& lua)
{
    ImGuiAPI::Unregister(lua);
    lua["console"] = sol::nil;
    lua["menu"]    = sol::nil;
    lua["utils"]   = sol::nil;
    lua["filesystem"] = sol::nil;
    lua["logger"]     = sol::nil;
}
