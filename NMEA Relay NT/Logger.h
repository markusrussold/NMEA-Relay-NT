#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <mutex>
#include <string>

class Logger {
private:
    std::ofstream logFile;
    std::mutex logMutex;
    std::string ConvertWStringToString(const std::wstring& wstr) const;
    bool withTimeStamp = false;
    std::string GetExecutablePath();
    std::string fileNameAndPath = "";
    int log_level = 2; // Options: 0 = None, 1 = Error, 2 = All

public:
    enum LogLevel {
        LOG_OFF = 0,  // No logging
        LOG_ERROR = 1,  // Serious errors
        LOG_INFO = 2, // Info and Warnings
    };

    Logger(const std::string& filename, bool includeTimeStamp, const std::string& path);
    ~Logger();

    void SetLogLevel(const int newLogLevel);
    void LogMessage(const std::string& message, int msgLogLevel);
    void LogMessage(const std::wstring& message, int msgLogLevel);
    void LogMessage(const std::string& message);
    void LogMessage(const std::wstring& message);
    std::string GetFileNameAndPath();
};

#endif  // LOGGER_H