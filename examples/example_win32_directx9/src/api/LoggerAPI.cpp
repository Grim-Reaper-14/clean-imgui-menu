#include "LoggerAPI.hpp"

#include "FileSystemAPI.hpp"
#include "Lua_Manager.hpp"

#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

LoggerAPI& LoggerAPI::Instance()
{
    static LoggerAPI instance;
    return instance;
}

void LoggerAPI::SetLogFile(const std::string& path)
{
    if (path.empty())
        throw std::invalid_argument("Log file path cannot be empty");

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logFile = path;
}

std::string LoggerAPI::GetLogFile() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logFile;
}

void LoggerAPI::EnableFileLogging(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fileLoggingEnabled = enabled;
}

bool LoggerAPI::IsFileLoggingEnabled() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_fileLoggingEnabled;
}

bool LoggerAPI::ClearLogFile()
{
    std::string logFile;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        logFile = m_logFile;
    }

    try
    {
        FileSystemAPI::WriteFile(logFile, "");
        return true;
    }
    catch (const std::exception& error)
    {
        const std::string message =
            std::string("[LOGGER ERROR] Unable to clear log file: ") + error.what() + "\n";
        LuaManager::Instance().AppendOutput(message);
        ::OutputDebugStringA(message.c_str());
        return false;
    }
}

void LoggerAPI::SetMinimumLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minimumLevel = level;
}

LogLevel LoggerAPI::GetMinimumLevel() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_minimumLevel;
}

bool LoggerAPI::Log(LogLevel level, const std::string& message)
{
    std::string logFile;
    bool fileLoggingEnabled = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (level < m_minimumLevel)
            return true;

        logFile = m_logFile;
        fileLoggingEnabled = m_fileLoggingEnabled;
    }

    const std::string line =
        Timestamp() + " [" + LevelName(level) + "] " + message + "\n";

    LuaManager::Instance().AppendOutput(line);
    ::OutputDebugStringA(line.c_str());

    if (!fileLoggingEnabled)
        return true;

    try
    {
        FileSystemAPI::AppendFile(logFile, line);
        return true;
    }
    catch (const std::exception& error)
    {
        const std::string failure =
            std::string("[LOGGER ERROR] Unable to write log file: ") + error.what() + "\n";
        LuaManager::Instance().AppendOutput(failure);
        ::OutputDebugStringA(failure.c_str());
        return false;
    }
}

bool LoggerAPI::Trace(const std::string& message)
{
    return Log(LogLevel::Trace, message);
}

bool LoggerAPI::Debug(const std::string& message)
{
    return Log(LogLevel::Debug, message);
}

bool LoggerAPI::Info(const std::string& message)
{
    return Log(LogLevel::Info, message);
}

bool LoggerAPI::Warning(const std::string& message)
{
    return Log(LogLevel::Warning, message);
}

bool LoggerAPI::Error(const std::string& message)
{
    return Log(LogLevel::Error, message);
}

bool LoggerAPI::Critical(const std::string& message)
{
    return Log(LogLevel::Critical, message);
}

const char* LoggerAPI::LevelName(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:    return "TRACE";
    case LogLevel::Debug:    return "DEBUG";
    case LogLevel::Info:     return "INFO";
    case LogLevel::Warning:  return "WARN";
    case LogLevel::Error:    return "ERROR";
    case LogLevel::Critical: return "CRITICAL";
    default:                 return "UNKNOWN";
    }
}

bool LoggerAPI::TryParseLevel(const std::string& value, LogLevel& level)
{
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

    if (normalized == "trace")    level = LogLevel::Trace;
    else if (normalized == "debug")    level = LogLevel::Debug;
    else if (normalized == "info")     level = LogLevel::Info;
    else if (normalized == "warn" || normalized == "warning")
        level = LogLevel::Warning;
    else if (normalized == "error")    level = LogLevel::Error;
    else if (normalized == "critical") level = LogLevel::Critical;
    else return false;

    return true;
}

std::string LoggerAPI::Timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}
