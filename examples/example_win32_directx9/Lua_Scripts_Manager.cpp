#include "Lua_Scripts_Manager.hpp"

// Windows-native directory enumeration (no extra dependencies).
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>

// ---------------------------------------------------------------------------
LuaScriptsManager::LuaScriptsManager(const std::string& scriptsDirectory)
    : m_directory(scriptsDirectory)
{
}

// ---------------------------------------------------------------------------
void LuaScriptsManager::SetDirectory(const std::string& dir)
{
    m_directory = dir;
    ScanDirectory();
}

// ---------------------------------------------------------------------------
int LuaScriptsManager::ScanDirectory()
{
    m_scriptFiles.clear();
    m_scriptNames.clear();

    std::string pattern = m_directory + "\\*.lua";

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);

    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    do
    {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::string name = ffd.cFileName;
            std::string full = m_directory + "\\" + name;
            m_scriptFiles.push_back(full);
            m_scriptNames.push_back(name);
        }
    }
    while (FindNextFileA(hFind, &ffd) != 0);

    FindClose(hFind);
    return static_cast<int>(m_scriptFiles.size());
}

// ---------------------------------------------------------------------------
bool LuaScriptsManager::EnsureDirectoryExists() const
{
    DWORD attr = GetFileAttributesA(m_directory.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return true;

    return CreateDirectoryA(m_directory.c_str(), nullptr) != 0;
}

// ---------------------------------------------------------------------------
bool LuaScriptsManager::FileExists(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}
