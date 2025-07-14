#include "pch.h"
#include "AppConfig.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Globals.h"
#include "MainWindow.xaml.h"
#include "Constants.h"

using namespace winrt::NMEA_Relay_NT; // <-- GANZ WICHTIG!

// Constructor
AppConfig::AppConfig(const std::string& file) : filename(file) {}

// Destructor
AppConfig::~AppConfig() {
    Save();
}

// Laden
bool AppConfig::Load() {
    std::lock_guard<std::mutex> lock(configMutex);

    std::ifstream infile(filename);
    if (!infile.is_open()) {
        g_loggerEvents.LogMessage("Could not open config file: " + filename, 1);
        return false;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "ship_name") ship_name = value;
            else if (key == "ship_dest") ship_dest = value;
            else if (key == "server_name") server_name = value;
            else if (key == "server_port") server_port = std::stoi(value);
            else if (key == "ship_status") ship_status = std::stoi(value);
            else if (key == "api_key") api_key = value;
            else if (key == "sailing_message") sailing_message = value;
            else if (key == "engine_message") engine_message = value;
            else if (key == "callSign") callSign = value;
            else if (key == "OpenCpnServer") OpenCpnServer = value;
            else if (key == "OpenCpnPort") OpenCpnPort = std::stoi(value);
            else if (key == "log_event_level") log_event_level = std::stoi(value);
            else if (key == "log_gpsdata") log_gpsdata = std::stoi(value);
            else if (key == "gps_max_data_age_seconds") gps_max_data_age_seconds = std::stoi(value);
            else if (key == "send_pos_reports") send_pos_reports = std::stoi(value);
            else if (key == "tripdist") tripdist = std::stod(value);
        }
    }

    infile.close();
    UpdateLoggers();
    return true;
}

// Speichern
bool AppConfig::Save() {
    std::lock_guard<std::mutex> lock(configMutex);

    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        g_loggerEvents.LogMessage("Could not open config file for writing: " + filename, 1);
        return false;
    }

    outfile << "ship_name=" << ship_name << "\n";
    outfile << "ship_dest=" << ship_dest << "\n";
    outfile << "server_name=" << server_name << "\n";
    outfile << "server_port=" << server_port << "\n";
    outfile << "ship_status=" << ship_status << "\n";
    outfile << "api_key=" << api_key << "\n";
    outfile << "sailing_message=" << sailing_message << "\n";
    outfile << "engine_message=" << engine_message << "\n";
    outfile << "callSign=" << callSign << "\n";
    outfile << "OpenCpnServer=" << OpenCpnServer << "\n";
    outfile << "OpenCpnPort=" << OpenCpnPort << "\n";
    outfile << "log_event_level=" << log_event_level << "\n";
    outfile << "log_gpsdata=" << log_gpsdata << "\n";
    outfile << "gps_max_data_age_seconds=" << gps_max_data_age_seconds << "\n";
    outfile << "send_pos_reports=" << send_pos_reports << "\n";
    outfile << "tripdist=" << tripdist << "\n";

    outfile.close();
    return true;
}

double AppConfig::GetTripDistance() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return tripdist;
}

void AppConfig::SetTripDistance(double value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        tripdist = value;
    }
    Save();
}

// Ship Name
std::string AppConfig::GetShipName() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return ship_name;
}

void AppConfig::SetShipName(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        ship_name = value;
    }
    Save();
}

// Ship Destination
std::string AppConfig::GetShipDest() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return ship_dest;
}

void AppConfig::SetShipDest(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        ship_dest = value;
    }
    Save();
}

// Server Name
std::string AppConfig::GetServerName() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return server_name;
}

void AppConfig::SetServerName(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        server_name = value;
    }
    Save();
}

// Server Port
int AppConfig::GetServerPort() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return server_port;
}

void AppConfig::SetServerPort(int value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        server_port = value;
    }
    Save();
}

// Ship Status
int AppConfig::GetShipStatus() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return ship_status;
}

void AppConfig::SetShipStatus(int value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        ship_status = value;
    }
    Save();
}

// API Key
std::string AppConfig::GetApiKey() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return api_key;
}

void AppConfig::SetApiKey(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        api_key = value;
    }
    Save();
}

// Sailing Message
std::string AppConfig::GetSailingMessage() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return sailing_message;
}

void AppConfig::SetSailingMessage(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        sailing_message = value;
    }
    Save();
}

// Engine Message
std::string AppConfig::GetEngineMessage() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return engine_message;
}

void AppConfig::SetEngineMessage(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        engine_message = value;
    }
    Save();
}

// Call Sign
std::string AppConfig::GetCallSign() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return callSign;
}

void AppConfig::SetCallSign(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        callSign = value;
    }
    Save();
}

// OpenCPN Server
std::string AppConfig::GetOpenCpnServer() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return OpenCpnServer;
}

void AppConfig::SetOpenCpnServer(const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        OpenCpnServer = value;
    }
    Save();
}

// OpenCPN Port
int AppConfig::GetOpenCpnPort() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return OpenCpnPort;
}

void AppConfig::SetOpenCpnPort(int value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        OpenCpnPort = value;
    }
    Save();
}

// Log Event Level
int AppConfig::GetLogEventLevel() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return log_event_level;
}

void AppConfig::SetLogEventLevel(int value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        log_event_level = value;
    }
    Save();
}

// Log GPS Data
int AppConfig::GetLogGpsData() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return log_gpsdata;
}

void AppConfig::SetLogGpsData(int value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        log_gpsdata = value;
    }
    Save();
}

// GPS Max Data Age
int AppConfig::GetGpsMaxDataAgeSeconds() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return gps_max_data_age_seconds;
}

void AppConfig::SetGpsMaxDataAgeSeconds(int value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        gps_max_data_age_seconds = value;
    }
    Save();
}

// Send Pos Reports
int AppConfig::GetSendPosReports() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return send_pos_reports;
}

void AppConfig::SetSendPosReports(int value) {
    {
        std::lock_guard<std::mutex> lock(configMutex);
        send_pos_reports = value;
    }
    Save();
}

void AppConfig::UpdateLoggers() {
    g_loggerEvents.SetLogLevel(log_event_level);
    g_loggerGPS.SetLogLevel(log_gpsdata == 0 ? 0 : 2);
}

void AppConfig::ApplyToWindow()
{
    auto myship_name = ship_name;
    auto myship_dest = ship_dest;
    auto myserver_name = server_name;
    auto myserver_port = server_port;
    auto myapi_key = api_key;
    auto mycallSign = callSign;
    auto myOpenCpnServer = OpenCpnServer;
    auto myOpenCpnPort = OpenCpnPort;
    auto myship_status = ship_status;
    auto mytripdist = tripdist;

    auto dispatcher = g_mainWindow.DispatcherQueue();
    dispatcher.TryEnqueue([myship_name, myship_dest, myserver_name, myserver_port, myapi_key, mycallSign, myOpenCpnServer, myOpenCpnPort, myship_status, mytripdist]() {
        auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);

        impl->SetButtonStatus(myship_status);
        impl->SetBoatName(winrt::to_hstring(myship_name));
        impl->SetDestination(winrt::to_hstring(myship_dest));
        impl->SetCallsign(winrt::to_hstring(mycallSign));
        impl->SetServerName(winrt::to_hstring(myserver_name));
        impl->SetServerPort(winrt::to_hstring(std::to_string(myserver_port)));
        impl->SetKey(winrt::to_hstring(myapi_key));
        impl->SetOpenCPN(winrt::to_hstring(myOpenCpnServer));
        impl->SetOpenCPNPort(winrt::to_hstring(std::to_string(myOpenCpnPort)));
        impl->SetTripDistanceText(mytripdist);
    });
}

