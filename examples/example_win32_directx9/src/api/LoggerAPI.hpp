#pragma once

#include <mutex>
#include <string>

enum class LogLevel
{
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

// Timestamped application logger. Every accepted message is written to the
// menu's output buffer and debugger; file logging can be enabled independently.
class LoggerAPI
{
public:
    static LoggerAPI& Instance();

    void SetLogFile(const std::string& path);
    std::string GetLogFile() const;

    void EnableFileLogging(bool enabled);
    bool IsFileLoggingEnabled() const;
    bool ClearLogFile();

    void SetMinimumLevel(LogLevel level);
    LogLevel GetMinimumLevel() const;

    bool Log(LogLevel level, const std::string& message);
    bool Trace(const std::string& message);
    bool Debug(const std::string& message);
    bool Info(const std::string& message);
    bool Warning(const std::string& message);
    bool Error(const std::string& message);
    bool Critical(const std::string& message);

    static const char* LevelName(LogLevel level);
    static bool TryParseLevel(const std::string& value, LogLevel& level);

private:
    LoggerAPI() = default;
    LoggerAPI(const LoggerAPI&) = delete;
    LoggerAPI& operator=(const LoggerAPI&) = delete;

    static std::string Timestamp();

    mutable std::mutex m_mutex;
    std::string m_logFile = "logs/application.log";
    bool m_fileLoggingEnabled = true;
    LogLevel m_minimumLevel = LogLevel::Trace;
};
