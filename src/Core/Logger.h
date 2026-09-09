#pragma once
#include "Types.h"
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace InspectionApp {

    enum class LogLevel {
        Trace, Debug, Info, Warning, Error, Fatal
    };

    class Logger {
    public:
        static Logger& Instance();
        void Init(const std::string& logFile);
        void Log(LogLevel level, const std::string& message, const char* file, int line);
        void SetConsoleOutput(bool enabled) { m_consoleOutput = enabled; }
        void SetFileOutput(bool enabled) { m_fileOutput = enabled; }

    private:
        Logger() = default;
        ~Logger();
        std::string LevelToString(LogLevel level);
        std::string CurrentTimestamp();

        std::ofstream m_file;
        std::mutex m_mutex;
        bool m_consoleOutput = true;
        bool m_fileOutput = true;
        bool m_initialized = false;
    };

#define LOG_TRACE(msg)  InspectionApp::Logger::Instance().Log(InspectionApp::LogLevel::Trace,  msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg)  InspectionApp::Logger::Instance().Log(InspectionApp::LogLevel::Debug,  msg, __FILE__, __LINE__)
#define LOG_INFO(msg)   InspectionApp::Logger::Instance().Log(InspectionApp::LogLevel::Info,   msg, __FILE__, __LINE__)
#define LOG_WARN(msg)   InspectionApp::Logger::Instance().Log(InspectionApp::LogLevel::Warning,msg, __FILE__, __LINE__)
#define LOG_ERROR(msg)  InspectionApp::Logger::Instance().Log(InspectionApp::LogLevel::Error,  msg, __FILE__, __LINE__)
#define LOG_FATAL(msg)  InspectionApp::Logger::Instance().Log(InspectionApp::LogLevel::Fatal,  msg, __FILE__, __LINE__)

} // namespace InspectionApp