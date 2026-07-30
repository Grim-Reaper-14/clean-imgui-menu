#pragma once
#include <string>

// lua_State is forward-declared so this header compiles without the Lua SDK
// on the include path. The .cpp includes lua.h / lauxlib.h directly.
struct lua_State;

enum class LuaScriptStatus
{
    Stopped = 0,
    Running,
    Error
};

// Represents a single .lua script loaded into a shared lua_State.
class LuaModule
{
public:
    LuaModule(const std::string& name, const std::string& filePath);
    ~LuaModule();

    // Load (compile) the script. Returns false on syntax error.
    bool Load(lua_State* L);

    // Execute the previously loaded script chunk.
    bool Execute(lua_State* L);

    // Reload: unload then load+execute.
    bool Reload(lua_State* L);

    // Mark the module as not loaded; does not close the state.
    void Unload();

    // ---- accessors ----
    const std::string& GetName()      const { return m_name; }
    const std::string& GetFilePath()  const { return m_filePath; }
    const std::string& GetAuthor()    const { return m_author; }
    const std::string& GetVersion()   const { return m_version; }
    const std::string& GetLastError() const { return m_lastError; }
    LuaScriptStatus    GetStatus()    const { return m_status; }
    bool               IsEnabled()    const { return m_enabled; }
    bool               IsLoaded()     const { return m_loaded; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    void SetAuthor(const std::string& author)   { m_author  = author;  }
    void SetVersion(const std::string& version) { m_version = version; }

private:
    std::string     m_name;
    std::string     m_filePath;
    std::string     m_author   = "Unknown";
    std::string     m_version  = "1.0";
    std::string     m_lastError;
    LuaScriptStatus m_status   = LuaScriptStatus::Stopped;
    bool            m_enabled  = true;
    bool            m_loaded   = false;
};
