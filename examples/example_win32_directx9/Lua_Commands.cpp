#include "Lua_Commands.hpp"
#include <algorithm>

// ---------------------------------------------------------------------------
void LuaCommands::RegisterCommand(const std::string& name,
                                   const std::string& description,
                                   std::function<std::string(lua_State*)> fn)
{
    LuaCommand* existing = FindCommand(name);
    if (existing)
    {
        existing->description = description;
        existing->execute     = std::move(fn);
        return;
    }

    LuaCommand cmd;
    cmd.name        = name;
    cmd.description = description;
    cmd.execute     = std::move(fn);
    m_commands.push_back(std::move(cmd));
}

// ---------------------------------------------------------------------------
bool LuaCommands::UnregisterCommand(const std::string& name)
{
    auto it = std::find_if(m_commands.begin(), m_commands.end(),
        [&name](const LuaCommand& c) { return c.name == name; });

    if (it == m_commands.end())
        return false;

    m_commands.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
std::string LuaCommands::ExecuteCommand(const std::string& name, lua_State* L)
{
    LuaCommand* cmd = FindCommand(name);
    if (!cmd)
        return "[ERROR] Unknown command: " + name;

    return cmd->execute(L);
}

// ---------------------------------------------------------------------------
bool LuaCommands::HasCommand(const std::string& name) const
{
    return FindCommand(name) != nullptr;
}

// ---------------------------------------------------------------------------
void LuaCommands::RegisterBuiltins()
{
    RegisterCommand("version", "Print the Lua system version",
        [](lua_State*) -> std::string { return "Lua 5.4 scripting system v1.0"; });

    RegisterCommand("list", "List all registered commands",
        [this](lua_State*) -> std::string
        {
            std::string out = "Registered commands:\n";
            for (const auto& c : m_commands)
                out += "  " + c.name + " - " + c.description + "\n";
            return out;
        });

    RegisterCommand("clear", "Clear the output buffer",
        [](lua_State*) -> std::string { return "__CLEAR__"; });
}

// ---------------------------------------------------------------------------
LuaCommand* LuaCommands::FindCommand(const std::string& name)
{
    for (auto& c : m_commands)
    {
        if (c.name == name)
            return &c;
    }
    return nullptr;
}

const LuaCommand* LuaCommands::FindCommand(const std::string& name) const
{
    for (const auto& c : m_commands)
    {
        if (c.name == name)
            return &c;
    }
    return nullptr;
}
