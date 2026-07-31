#include "FileSystemAPI.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace
{
    namespace fs = std::filesystem;

    fs::path ToPath(const std::string& path)
    {
        if (path.empty())
            throw std::invalid_argument("Filesystem path cannot be empty");

        return fs::path(reinterpret_cast<const char8_t*>(path.data()),
                        reinterpret_cast<const char8_t*>(path.data() + path.size()));
    }

    std::string ToUtf8(const fs::path& path)
    {
        const auto u8str = path.u8string();
        return std::string(reinterpret_cast<const char*>(u8str.data()), u8str.size());
    }

    [[noreturn]] void ThrowFileError(const char* operation, const fs::path& path,
                                     const std::error_code& error)
    {
        throw std::runtime_error(
            std::string(operation) + " failed for '" + ToUtf8(path) +
            "': " + error.message());
    }

    void CreateParentDirectories(const fs::path& path)
    {
        const fs::path parent = path.parent_path();
        if (parent.empty())
            return;

        std::error_code error;
        fs::create_directories(parent, error);
        if (error)
            ThrowFileError("Creating parent directory", parent, error);
    }

    void WriteFileImpl(const std::string& path, const std::string& contents,
                       std::ios::openmode mode)
    {
        const fs::path filePath = ToPath(path);
        CreateParentDirectories(filePath);

        std::ofstream file(filePath, std::ios::binary | mode);
        if (!file)
            throw std::runtime_error("Unable to open '" + path + "' for writing");

        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        if (!file)
            throw std::runtime_error("Unable to write to '" + path + "'");
    }
}

bool FileSystemAPI::Exists(const std::string& path)
{
    std::error_code error;
    const bool result = fs::exists(ToPath(path), error);
    if (error)
        ThrowFileError("Checking path", ToPath(path), error);
    return result;
}

bool FileSystemAPI::IsFile(const std::string& path)
{
    std::error_code error;
    const bool result = fs::is_regular_file(ToPath(path), error);
    if (error)
        ThrowFileError("Checking file", ToPath(path), error);
    return result;
}

bool FileSystemAPI::IsDirectory(const std::string& path)
{
    std::error_code error;
    const bool result = fs::is_directory(ToPath(path), error);
    if (error)
        ThrowFileError("Checking directory", ToPath(path), error);
    return result;
}

std::string FileSystemAPI::ReadFile(const std::string& path)
{
    const fs::path filePath = ToPath(path);
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Unable to open '" + path + "' for reading");

    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad())
        throw std::runtime_error("Unable to read from '" + path + "'");

    return contents.str();
}

void FileSystemAPI::WriteFile(const std::string& path, const std::string& contents)
{
    WriteFileImpl(path, contents, std::ios::trunc);
}

void FileSystemAPI::AppendFile(const std::string& path, const std::string& contents)
{
    WriteFileImpl(path, contents, std::ios::app);
}

bool FileSystemAPI::CreateDirectories(const std::string& path)
{
    const fs::path directory = ToPath(path);
    std::error_code error;
    const bool created = fs::create_directories(directory, error);
    if (error)
        ThrowFileError("Creating directory", directory, error);
    return created;
}

bool FileSystemAPI::Remove(const std::string& path, bool recursive)
{
    const fs::path target = ToPath(path);
    std::error_code error;
    const std::uintmax_t removed =
        recursive ? fs::remove_all(target, error)
                  : static_cast<std::uintmax_t>(fs::remove(target, error));
    if (error)
        ThrowFileError("Removing path", target, error);
    return removed > 0;
}

void FileSystemAPI::Copy(const std::string& source, const std::string& destination,
                         bool overwrite)
{
    const fs::path sourcePath = ToPath(source);
    const fs::path destinationPath = ToPath(destination);
    CreateParentDirectories(destinationPath);

    fs::copy_options options = fs::copy_options::recursive;
    options |= overwrite ? fs::copy_options::overwrite_existing
                         : fs::copy_options::none;

    std::error_code error;
    fs::copy(sourcePath, destinationPath, options, error);
    if (error)
        ThrowFileError("Copying path", sourcePath, error);
}

void FileSystemAPI::Move(const std::string& source, const std::string& destination,
                         bool overwrite)
{
    const fs::path sourcePath = ToPath(source);
    const fs::path destinationPath = ToPath(destination);
    CreateParentDirectories(destinationPath);

    std::error_code error;
    if (!fs::exists(sourcePath, error))
    {
        if (error)
            ThrowFileError("Checking move source", sourcePath, error);
        throw std::runtime_error("Move source does not exist: '" + source + "'");
    }

    const fs::path absoluteSource = fs::absolute(sourcePath, error).lexically_normal();
    if (error)
        ThrowFileError("Resolving move source", sourcePath, error);

    const fs::path absoluteDestination =
        fs::absolute(destinationPath, error).lexically_normal();
    if (error)
        ThrowFileError("Resolving move destination", destinationPath, error);

    if (absoluteSource == absoluteDestination)
        return;

    if (overwrite && fs::exists(destinationPath, error))
    {
        if (error)
            ThrowFileError("Checking move destination", destinationPath, error);

        fs::remove_all(destinationPath, error);
        if (error)
            ThrowFileError("Replacing move destination", destinationPath, error);
    }

    fs::rename(sourcePath, destinationPath, error);
    if (error)
        ThrowFileError("Moving path", sourcePath, error);
}

std::vector<std::string> FileSystemAPI::ListDirectory(const std::string& path,
                                                      bool recursive)
{
    const fs::path directory = ToPath(path);
    std::error_code error;
    if (!fs::is_directory(directory, error))
    {
        if (error)
            ThrowFileError("Checking directory", directory, error);
        throw std::runtime_error("'" + path + "' is not a directory");
    }

    std::vector<std::string> entries;
    const auto options = fs::directory_options::skip_permission_denied;

    if (recursive)
    {
        for (fs::recursive_directory_iterator it(directory, options, error), end;
             it != end; it.increment(error))
        {
            if (error)
                ThrowFileError("Listing directory", directory, error);
            entries.push_back(ToUtf8(it->path()));
        }
    }
    else
    {
        for (fs::directory_iterator it(directory, options, error), end;
             it != end; it.increment(error))
        {
            if (error)
                ThrowFileError("Listing directory", directory, error);
            entries.push_back(ToUtf8(it->path()));
        }
    }

    std::sort(entries.begin(), entries.end());
    return entries;
}

std::uintmax_t FileSystemAPI::FileSize(const std::string& path)
{
    const fs::path filePath = ToPath(path);
    std::error_code error;
    const std::uintmax_t size = fs::file_size(filePath, error);
    if (error)
        ThrowFileError("Reading file size", filePath, error);
    return size;
}

std::string FileSystemAPI::CurrentDirectory()
{
    std::error_code error;
    const fs::path path = fs::current_path(error);
    if (error)
        ThrowFileError("Reading current directory", fs::path("."), error);
    return ToUtf8(path);
}

std::string FileSystemAPI::AbsolutePath(const std::string& path)
{
    const fs::path input = ToPath(path);
    std::error_code error;
    const fs::path absolute = fs::absolute(input, error).lexically_normal();
    if (error)
        ThrowFileError("Resolving absolute path", input, error);
    return ToUtf8(absolute);
}

std::string FileSystemAPI::Join(const std::string& left, const std::string& right)
{
    return ToUtf8((ToPath(left) / ToPath(right)).lexically_normal());
}

std::string FileSystemAPI::FileName(const std::string& path)
{
    return ToUtf8(ToPath(path).filename());
}

std::string FileSystemAPI::Extension(const std::string& path)
{
    return ToUtf8(ToPath(path).extension());
}

std::string FileSystemAPI::ParentPath(const std::string& path)
{
    return ToUtf8(ToPath(path).parent_path());
}
