#pragma once

#include <string>
#include <windows.h>
#include <sstream>
#include <set>

// Declaration
std::string getRoamingAppDataFolder();
bool isServerAvailable(const std::string& ipAddressOrHostname);

void PipeServerLoop();

void appPulse();

void queueProcessing();

// Hilfsfunktion für Variadic Templates
template<typename T>
void appendToStream(std::ostringstream& oss, T value)
{
    oss << value;
}

template<typename T, typename... Args>
void appendToStream(std::ostringstream& oss, T value, Args... args)
{
    oss << value;
    appendToStream(oss, args...);
}

// Deine Debug-Log-Funktion
template<typename... Args>
void logToDebugger(Args... args)
{
#ifdef _DEBUG
    std::ostringstream oss;
    (oss << ... << args); // fold expression, C++17
    oss << "\n";
    OutputDebugStringA(oss.str().c_str());
#endif
}

void InitializeWinsock();

void CenterWindow(winrt::Microsoft::UI::Xaml::Window const& window);

void send_udp_message(const std::string& server, int port, const std::string& message);

std::string RemoveSpecialCharacters(const std::string& input);

std::wstring RemoveSpecialCharacters(const std::wstring& input);

void DecodeAndSendForeignPOSReport(const char* buffer);

bool isInSet(const std::set<std::string>& list, const std::string& messageType);

std::wstring ConvertToDMS(double decimalDegrees, bool isLatitude);

SOCKET tcp_connect_to_server(const char* server_hostname, int server_port);

int sendAISMessagePosition(std::string foreignPosReport);

std::string WStringToUtf8(const std::wstring& wstr);

double DegreesToRadians(double degrees);

double CalculateDistanceNm(double lat1, double lon1, double lat2, double lon2);

std::string FormatDoubleForGermanLocale(double value, int precision);

std::wstring FormatDoubleForGermanLocaleW(double value, int precision);
