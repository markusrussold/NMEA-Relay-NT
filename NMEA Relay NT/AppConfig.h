#pragma once

#include "pch.h"
#include <string>
#include <mutex>
#include "MainWindow.xaml.h"
#include "Constants.h"

class AppConfig {
public:
    AppConfig(const std::string& filename);
    ~AppConfig();

    bool Load();
    bool Save();

    double GetTripDistance() const;

    void SetTripDistance(double value);

    void ApplyToWindow();

    // Ship Name
    std::string GetShipName() const;
    void SetShipName(const std::string& value);

    // Ship Destination
    std::string GetShipDest() const;
    void SetShipDest(const std::string& value);

    // Server Name
    std::string GetServerName() const;
    void SetServerName(const std::string& value);

    // Server Port
    int GetServerPort() const;
    void SetServerPort(int value);

    // Ship Status
    int GetShipStatus() const;
    void SetShipStatus(int value);

    // API Key
    std::string GetApiKey() const;
    void SetApiKey(const std::string& value);

    // Sailing Message
    std::string GetSailingMessage() const;
    void SetSailingMessage(const std::string& value);

    // Engine Message
    std::string GetEngineMessage() const;
    void SetEngineMessage(const std::string& value);

    // Call Sign
    std::string GetCallSign() const;
    void SetCallSign(const std::string& value);

    // OpenCPN Server
    std::string GetOpenCpnServer() const;
    void SetOpenCpnServer(const std::string& value);

    // OpenCPN Port
    int GetOpenCpnPort() const;
    void SetOpenCpnPort(int value);

    // Log Event Level
    int GetLogEventLevel() const;
    void SetLogEventLevel(int value);

    // Log GPS Data
    int GetLogGpsData() const;
    void SetLogGpsData(int value);

    // GPS Max Data Age
    int GetGpsMaxDataAgeSeconds() const;
    void SetGpsMaxDataAgeSeconds(int value);

    // Send Position Reports
    int GetSendPosReports() const;
    void SetSendPosReports(int value);

private:
    mutable std::mutex configMutex;
    std::string filename;

    // Die Felder selbst
    std::string ship_name = "";
    std::string ship_dest = "";
    std::string server_name = "mrussold.com";
    int server_port = 1234;
    int ship_status = 0;
    std::string api_key = "";
    std::string sailing_message = "";
    std::string engine_message = "";
    std::string callSign = "";
    std::string OpenCpnServer = "127.0.0.1";
    int OpenCpnPort = 2947;
    int log_event_level = 2;
    int log_gpsdata = 1;
    int gps_max_data_age_seconds = MAX_DATA_AGE_SECONDS;
    int send_pos_reports = 1;
    double tripdist = 0.0;

    void UpdateLoggers();
};
