#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Small, dependency-free filesystem service used by both the native program
// and the Lua bindings. Paths passed by Lua are interpreted as UTF-8.
class FileSystemAPI
{
public:
    static bool Exists(const std::string& path);
    static bool IsFile(const std::string& path);
    static bool IsDirectory(const std::string& path);

    static std::string ReadFile(const std::string& path);
    static void WriteFile(const std::string& path, const std::string& contents);
    static void AppendFile(const std::string& path, const std::string& contents);

    static bool CreateDirectories(const std::string& path);
    static bool Remove(const std::string& path, bool recursive = false);
    static void Copy(const std::string& source, const std::string& destination,
                     bool overwrite = true);
    static void Move(const std::string& source, const std::string& destination,
                     bool overwrite = true);

    static std::vector<std::string> ListDirectory(const std::string& path,
                                                  bool recursive = false);
    static std::uintmax_t FileSize(const std::string& path);

    static std::string CurrentDirectory();
    static std::string AbsolutePath(const std::string& path);
    static std::string Join(const std::string& left, const std::string& right);
    static std::string FileName(const std::string& path);
    static std::string Extension(const std::string& path);
    static std::string ParentPath(const std::string& path);
};
