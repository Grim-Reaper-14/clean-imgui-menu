#pragma once
#include <string>
#include <vector>

// Handles discovery and file management of .lua scripts on disk.
// Uses the Windows file API so no extra dependencies are required.
class LuaScriptsManager
{
public:
    explicit LuaScriptsManager(const std::string& scriptsDirectory = "scripts");

    // Change the watched directory and re-scan.
    void SetDirectory(const std::string& dir);
    const std::string& GetDirectory() const { return m_directory; }

    // Scan the directory and refresh the internal file list.
    // Returns the number of .lua files found.
    int ScanDirectory();

    // Paths of every .lua file found during the last scan.
    const std::vector<std::string>& GetScriptFiles()  const { return m_scriptFiles; }

    // Just the base file names (e.g. "aimbot.lua").
    const std::vector<std::string>& GetScriptNames()  const { return m_scriptNames; }

    int GetScriptCount() const { return static_cast<int>(m_scriptFiles.size()); }

    // Create the scripts directory if it does not exist.
    bool EnsureDirectoryExists() const;

    // True if the file exists on disk.
    static bool FileExists(const std::string& path);

private:
    std::string              m_directory;
    std::vector<std::string> m_scriptFiles;   // full paths
    std::vector<std::string> m_scriptNames;   // base names
};
