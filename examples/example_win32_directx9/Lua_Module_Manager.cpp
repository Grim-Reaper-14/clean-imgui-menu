#include "Lua_Module_Manager.hpp"
#include <algorithm>
#include <filesystem>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string BaseNameFromPath(const std::string& path)
{
    // Extract the file name without directory prefix
    std::filesystem::path p(path);
    return p.filename().string();
}

// ---------------------------------------------------------------------------
LuaModule* LuaModuleManager::AddModule(const std::string& filePath)
{
    std::string name = BaseNameFromPath(filePath);

    // Do not add duplicates
    for (auto& mod : m_modules)
    {
        if (mod.GetName() == name)
            return &mod;
    }

    m_modules.emplace_back(name, filePath);
    return &m_modules.back();
}

// ---------------------------------------------------------------------------
bool LuaModuleManager::RemoveModule(const std::string& name)
{
    auto it = std::find_if(m_modules.begin(), m_modules.end(),
        [&name](const LuaModule& m) { return m.GetName() == name; });

    if (it == m_modules.end())
        return false;

    it->Unload();
    m_modules.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
LuaModule* LuaModuleManager::GetModule(const std::string& name)
{
    for (auto& mod : m_modules)
    {
        if (mod.GetName() == name)
            return &mod;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
void LuaModuleManager::LoadAll(lua_State* L)
{
    for (auto& mod : m_modules)
    {
        if (mod.IsEnabled() && !mod.IsLoaded())
            mod.Load(L);
    }
}

// ---------------------------------------------------------------------------
void LuaModuleManager::ExecuteAll(lua_State* L)
{
    for (auto& mod : m_modules)
    {
        if (mod.IsEnabled() && mod.IsLoaded())
            mod.Execute(L);
    }
}

// ---------------------------------------------------------------------------
void LuaModuleManager::ReloadAll(lua_State* L)
{
    for (auto& mod : m_modules)
    {
        if (mod.IsEnabled())
            mod.Reload(L);
    }
}

// ---------------------------------------------------------------------------
void LuaModuleManager::Clear()
{
    for (auto& mod : m_modules)
        mod.Unload();
    m_modules.clear();
}
