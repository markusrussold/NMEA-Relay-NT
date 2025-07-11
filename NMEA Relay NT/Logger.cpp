#include "pch.h"
#include "Logger.h"
#include <locale>
#include <codecvt>
#include <windows.h>  // For WideCharToMultiByte and CP_UTF8 on Windows
#include <iostream>
#include <ctime>
#include <chrono>
#include <iomanip>

// Function to get the executable's directory path
std::string Logger::GetExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);  // Extract directory path
}

// Constructor: Opens the log file
Logger::Logger(const std::string& filename, bool includeTimeStamp, const std::string& path) {
    withTimeStamp = includeTimeStamp;

    // Determine the log file path based on whether path is provided or not
    std::string logFilePath = (path.empty()) ? (GetExecutablePath() + "\\" + filename) : (path + "\\" + filename);

    fileNameAndPath = logFilePath;

    // Open the log file
    logFile.open(logFilePath, std::ios::out | std::ios::app);  // Append mode
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file at " << logFilePath << std::endl;
    }
}

// Destructor: Closes the log file with a final shutdown message
Logger::~Logger() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        if (withTimeStamp) {
            auto now = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
            std::tm localTime;
            localtime_s(&localTime, &currentTime);
            logFile << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "\t";
        }

        logFile << "Logger is shutting down" << std::endl;
        logFile.close();
    }
}

// Converts std::wstring to std::string (UTF-8)
std::string Logger::ConvertWStringToString(const std::wstring& wstr) const {
    if (wstr.empty()) {
        return std::string();
    }

    // Get the size needed for the buffer
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded == 0) {
        // Handle the error (e.g., throw an exception or log it)
        return std::string();
    }

    // Allocate the buffer
    std::string result(sizeNeeded, 0);

    // Perform the conversion
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], sizeNeeded, nullptr, nullptr);

    // Remove the null terminator that was included in the size
    result.resize(sizeNeeded - 1);

    return result;
}

void Logger::LogMessage(const std::string& message, int msgLogLevel) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (msgLogLevel > 0 && msgLogLevel <= log_level) {
        if (logFile.is_open()) {
            if (withTimeStamp) {
                // Get current time as a time_t object
                auto now = std::chrono::system_clock::now();
                std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

                // Convert to local time and format it to a human-readable format
                std::tm localTime;
                localtime_s(&localTime, &currentTime);  // Thread-safe version of localtime

                logFile << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "\t";  // Append timestamp
            }
            logFile << message << std::endl;
        }
    }
}

// Log a message (wstring)
void Logger::LogMessage(const std::wstring& message, int msgLogLevel) {
    LogMessage(ConvertWStringToString(message), msgLogLevel);
}

void Logger::LogMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    int msgLogLevel = 2;

    if (msgLogLevel > 0 && msgLogLevel <= log_level) {
        if (logFile.is_open()) {
            if (withTimeStamp) {
                // Get current time as a time_t object
                auto now = std::chrono::system_clock::now();
                std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

                // Convert to local time and format it to a human-readable format
                std::tm localTime;
                localtime_s(&localTime, &currentTime);  // Thread-safe version of localtime

                logFile << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "\t";  // Append timestamp
            }
            logFile << message << std::endl;
        }
    }
}

// Log a message (wstring)
void Logger::LogMessage(const std::wstring& message) {
    LogMessage(ConvertWStringToString(message));
}

std::string Logger::GetFileNameAndPath() {
    return fileNameAndPath;
};

void Logger::SetLogLevel(const int newLogLevel) {
    if (newLogLevel >= 0 && newLogLevel <= 2) {
        log_level = newLogLevel;
    }
};