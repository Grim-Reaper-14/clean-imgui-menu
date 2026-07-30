#pragma once
#include "Lua_Module.hpp"
#include <vector>
#include <string>

struct lua_State;

// Owns and manages the lifetime of every LuaModule.
class LuaModuleManager
{
public:
    // Add a script by file path; name is derived from the file name.
    LuaModule* AddModule(const std::string& filePath);

    // Remove a module by name. Returns true if it was found and removed.
    bool RemoveModule(const std::string& name);

    // Find a module by name.  Returns nullptr if not found.
    LuaModule* GetModule(const std::string& name);

    // Load (compile) all modules that are enabled and not yet loaded.
    void LoadAll(lua_State* L);

    // Execute all enabled, loaded modules.
    void ExecuteAll(lua_State* L);

    // Reload every module (unload then load+execute).
    void ReloadAll(lua_State* L);

    // Unload and remove all modules.
    void Clear();

    // Read-only access to the module list for the UI.
    const std::vector<LuaModule>& GetModules() const { return m_modules; }
          std::vector<LuaModule>& GetModules()       { return m_modules; }

    int GetCount() const { return static_cast<int>(m_modules.size()); }

private:
    std::vector<LuaModule> m_modules;
};
