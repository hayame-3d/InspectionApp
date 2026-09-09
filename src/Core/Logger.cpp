#include "Logger.h"

namespace InspectionApp {

    Logger& Logger::Instance() {
        static Logger instance;
        return instance;
    }

    Logger::~Logger() {
        if (m_file.is_open()) m_file.close();
    }

    void Logger::Init(const std::string& logFile) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_file.open(logFile, std::ios::app);
        }
        Log(LogLevel::Info, "Logger initialized", __FILE__, __LINE__);
    }

    std::string Logger::LevelToString(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        }
        return "UNKNOWN";
    }

    std::string Logger::CurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void Logger::Log(LogLevel level, const std::string& message, const char* file, int line) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream oss;
        oss << "[" << CurrentTimestamp() << "] [" << LevelToString(level) << "] "
            << file << ":" << line << " - " << message;
        std::string out = oss.str();
        if (m_consoleOutput) {
            std::cout << out << std::endl;
        }
        if (m_fileOutput && m_file.is_open()) {
            m_file << out << std::endl;
        }
    }

} // namespace InspectionApp