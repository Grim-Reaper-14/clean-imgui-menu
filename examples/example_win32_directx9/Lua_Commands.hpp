#pragma once
#include <string>
#include <vector>
#include <functional>

struct lua_State;

// A named command that can be invoked from the menu UI or from a Lua script.
struct LuaCommand
{
    std::string                          name;
    std::string                          description;
    std::function<std::string(lua_State*)> execute; // returns output string
};

// Registry of all named commands. Commands can be added by C++ code or by
// binding helpers so Lua scripts can call back into the application.
class LuaCommands
{
public:
    // Register a new command.  Overwrites an existing command with the same name.
    void RegisterCommand(const std::string& name,
                         const std::string& description,
                         std::function<std::string(lua_State*)> fn);

    // Remove a command by name. Returns true if it was found.
    bool UnregisterCommand(const std::string& name);

    // Execute a command by name. Returns the command output, or an error string.
    std::string ExecuteCommand(const std::string& name, lua_State* L);

    // True if a command with the given name exists.
    bool HasCommand(const std::string& name) const;

    // Read-only view of all registered commands.
    const std::vector<LuaCommand>& GetCommands() const { return m_commands; }

    int GetCount() const { return static_cast<int>(m_commands.size()); }

    // Register the built-in commands (version, list, clear, reload_all).
    void RegisterBuiltins();

private:
    LuaCommand* FindCommand(const std::string& name);
    const LuaCommand* FindCommand(const std::string& name) const;

    std::vector<LuaCommand> m_commands;
};
