#include "pch.h"
#include "HelperFunctions.h"
#include <winrt/Windows.Storage.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <iostream>
#include "Globals.h"
#include <chrono>
#include "AppConfig.h"
#include "Constants.h"
#include <set>
#include "AisProcessor.h"
#include <windows.h>
#include <string>
#include <locale>
#include <codecvt>
#include <atomic>
#include <algorithm>
#include <winrt/Windows.System.Threading.h>
#include <winrt/Windows.Foundation.h>
#include <cmath>

#include <nmea/sentence.hpp>
#include <nmea/message/gga.hpp>
#include <nmea/message/gll.hpp>
#include <nmea/message/rmc.hpp>
#include <nmea/message/vtg.hpp>
#include <nmea/message/gsa.hpp>
#include <nmea/message/zda.hpp>

class MainDialog : public Observer {
private:
    gpsData& _gpsData;
    std::atomic<float> latestSog = 0.0f;
    std::atomic<float> latestCog = -1.0f;
    std::atomic<float> latestLat = 0.0f;
    std::atomic<float> latestLon = 0.0f;
    std::atomic<float> latestUTC = 0.0f;
    std::atomic<float> latestTripDist = 0.0f;
    std::atomic<bool> latestDataReliability = false;
    std::mutex _updateMutex;
    std::atomic<int> latestDataAge = 0;
    bool updateScheduled = false;

public:
    MainDialog(gpsData& gpsDataRef) : _gpsData(gpsDataRef) {
        _gpsData.Attach(this);  // Register as observer
    }

    ~MainDialog() {
        _gpsData.Detach(this);  // Clean up observer
    }

    void UpdateHdop(double hdop) override {}
    void UpdateAlt(double alt) override {}
    void UpdateFix(uint8_t fix) override {}
    void UpdateSatelliteCount(uint8_t SatelliteCount) override {}
    void UpdateRmcValidity(bool rmc_valid) override {}

    void UpdateDataAge(int newDataAge) override {
        latestDataAge.store(newDataAge, std::memory_order_relaxed);
        ScheduleUpdate();
    }

    void UpdateUtc(double utc) override
    {
        latestUTC.store(utc, std::memory_order_relaxed);
        ScheduleUpdate();
    }

    void UpdateTripDist(double tripdist) override
    {
        latestTripDist.store(tripdist, std::memory_order_relaxed);
        g_config.SetTripDistance(tripdist);
        ScheduleUpdate();

        winrt::Windows::System::Threading::ThreadPool::RunAsync([](winrt::Windows::Foundation::IAsyncAction const&) {
            g_config.Save();
        });
    }

    void UpdateLatLon(double lat, double lon) override
    {
        latestLat.store(lat, std::memory_order_relaxed);
        latestLon.store(lon, std::memory_order_relaxed);
        ScheduleUpdate();
    }

    void UpdateDataReliability(bool data_reliability) override {
        latestDataReliability.store(data_reliability, std::memory_order_relaxed);

        auto dispatcher = g_mainWindow.DispatcherQueue();
        dispatcher.TryEnqueue([this]() {
            auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
            impl->SetDataReliability(latestDataReliability.load(std::memory_order_relaxed));
            });
    }

    void UpdateSog(float sog) override {
        latestSog.store(sog, std::memory_order_relaxed);
        ScheduleUpdate();
    }

    void UpdateCog(float cog) override {
        latestCog.store(cog, std::memory_order_relaxed);
        ScheduleUpdate();
    }

    void ScheduleUpdate() {
        std::lock_guard lock(_updateMutex);
        if (!updateScheduled) {
            updateScheduled = true;

            auto dispatcher = g_mainWindow.DispatcherQueue();
            dispatcher.TryEnqueue([this]() {
                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                impl->SetSOG(latestSog.load(std::memory_order_relaxed));
                impl->SetCOG(latestCog.load(std::memory_order_relaxed));
                impl->SetLatLon(latestLat.load(std::memory_order_relaxed), latestLon.load(std::memory_order_relaxed));
                impl->SetUtc(latestUTC.load(std::memory_order_relaxed));
                impl->SetDataAge(latestDataAge.load(std::memory_order_relaxed));
                impl->SetTripDistanceText(latestTripDist.load(std::memory_order_relaxed));

                // Ready for next batch
                {
                    std::lock_guard innerLock(_updateMutex);
                    updateScheduled = false;
                }
            });
        }
    }
};

MainDialog dialog(GPSData);

// List of NMEA 0183 GPS messages (afer GP/GN prefix)
std::set<std::string> gpsMessages = {
    "GGA", "GLL", "GSA", "GSV", "RMC", "VTG", "ZDA", "GST"
};

// List of AIS messages (after "!AI" prefix)
std::set<std::string> aisMessages = {
    "VDM", "VDO", "ACA", "ALR", "BBM", "CBM", "LRF", "SSD", "TLL", "TLM", "TXT"
};

std::string getRoamingAppDataFolder()
{
    using namespace winrt::Windows::Storage;
    auto folder = ApplicationData::Current().RoamingFolder();
    return winrt::to_string(folder.Path());
}

bool isServerAvailable(const std::string& ipAddressOrHostname)
{
    // Resolve hostname to IP
    addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    if (getaddrinfo(ipAddressOrHostname.c_str(), nullptr, &hints, &result) != 0)
    {
        //g_loggerEvents.LogMessage("DNS failed : " + ipAddressOrHostname, Logger::LOG_ERROR);
        //logToDebugger("DNS failed: ", ipAddressOrHostname.c_str());
        return false; // DNS failed
    }

    sockaddr_in* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    IPAddr ip = sockaddr_ipv4->sin_addr.S_un.S_addr;

    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE)
    {
        logToDebugger("Invalid Handle Value");
        freeaddrinfo(result);
        return false;
    }

    constexpr int replySize = sizeof(ICMP_ECHO_REPLY) + 32;
    char replyBuffer[replySize] = { 0 };

    DWORD dwRetVal = IcmpSendEcho(
        hIcmpFile,
        ip,
        nullptr,
        0,
        nullptr,
        replyBuffer,
        replySize,
        1000 // Timeout in ms
    );

    IcmpCloseHandle(hIcmpFile);
    freeaddrinfo(result);

    return dwRetVal != 0;
}

//void PipeServerLoop()
//{
//    struct VesselState {
//        std::wstring latlon = L"39° 53' 34.00\" N   4° 16' 21.89\" E";
//        double cog = 120.5;
//        double tripdist = 34.261;
//        std::string status = "UNKNOWN";
//    };
//
//    g_loggerEvents.LogMessage("thread PipeServerLoopThread started", 2);
//    logToDebugger("thread PipeServerLoopThread started");
//
//    while (!g_shouldStopThreads)
//    {
//        HANDLE hPipe = CreateNamedPipe(
//            L"\\\\.\\pipe\\NMEA_PIPE",
//            PIPE_ACCESS_DUPLEX,
//            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
//            PIPE_UNLIMITED_INSTANCES,
//            512, 512,
//            0,
//            NULL
//        );
//
//        if (hPipe == INVALID_HANDLE_VALUE) {
//            logToDebugger("CreateNamedPipe failed. Error: ", GetLastError());
//            break;
//        }
//
//        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
//
//        if (connected) {
//            // Capture state per client
//            VesselState state;
//
//            // Lambda to handle the client
//            std::thread([hPipe, state]() mutable {
//                char buffer[128] = {};
//                DWORD bytesRead = 0;
//
//                BOOL result = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
//                if (!result || bytesRead == 0) {
//                    DisconnectNamedPipe(hPipe);
//                    CloseHandle(hPipe);
//                    return;
//                }
//
//                std::string command(buffer, bytesRead);
//                command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());
//                command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
//
//                logToDebugger("Pipe Command received: ", command);
//                DWORD bytesWritten = 0;
//
//                if (command == "GET_LATLON") {
//                    try {
//                        std::wstring positionText = L"";
//                        //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
//                        //std::string reply = converter.to_bytes(state.latlon);
//
//                        if (GPSData.GetDataReliability())
//                        {
//                            auto latDMS = ConvertToDMS(GPSData.GetLatitude(), true);
//                            auto lonDMS = ConvertToDMS(GPSData.GetLongitude(), false);
//
//                            positionText = latDMS + L"   " + lonDMS;
//                        }
//
//                        std::string reply = WStringToUtf8(positionText);
//
//                        WriteFile(hPipe, reply.c_str(), static_cast<DWORD>(reply.size()), &bytesWritten, NULL);
//                    }
//                    catch (const std::exception& e) {
//                        logToDebugger("Pipe Handler - UTF-8 encoding error: ", e.what());
//                    }
//                }
//                else if (command == "GET_COG") {
//                    double COGData = 0.0;
//
//                    if (GPSData.GetDataReliability()) {
//                        COGData = GPSData.GetCog();
//                    }
//
//                    std::string reply = FormatDoubleForGermanLocale(COGData, 4);
//                    WriteFile(hPipe, reply.c_str(), static_cast<DWORD>(reply.size()), &bytesWritten, NULL);
//                }
//                else if (command == "GET_TRIPDIST") {
//                    double tripDistData = 0.0;
//
//                    if (GPSData.GetDataReliability()) {
//                        tripDistData = GPSData.GetTripDist();
//                    }
//
//                    std::string reply = FormatDoubleForGermanLocale(tripDistData, 4);
//                    WriteFile(hPipe, reply.c_str(), static_cast<DWORD>(reply.size()), &bytesWritten, NULL);
//                }
//                else if (command == "SET_ENGINE") {
//                    if (g_mainWindow) {
//                        auto dispatcher = g_mainWindow.DispatcherQueue();
//                        dispatcher.TryEnqueue([]()
//                            {
//                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
//                                impl->EngineButton_Click(nullptr, nullptr);
//                            });
//                    }
//
//                    logToDebugger("Pipe Handler - Status updated to ENGINE");
//                }
//                else if (command == "SET_SAIL") {
//                    if (g_mainWindow) {
//                        auto dispatcher = g_mainWindow.DispatcherQueue();
//                        dispatcher.TryEnqueue([]()
//                            {
//                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
//                                impl->SailButton_Click(nullptr, nullptr);
//                            });
//                    }
//
//                    logToDebugger("Pipe Handler - Status updated to SAIL");
//                }
//                else if (command == "SET_DOCKED") {
//                    if (g_mainWindow) {
//                        auto dispatcher = g_mainWindow.DispatcherQueue();
//                        dispatcher.TryEnqueue([]()
//                            {
//                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
//                                impl->DockedButton_Click(nullptr, nullptr);
//                            });
//                    }
//
//                    logToDebugger("Pipe Handler - Status updated to DOCKED");
//                }
//                else if (command == "SET_ANCHOR") {
//                    if (g_mainWindow) {
//                        auto dispatcher = g_mainWindow.DispatcherQueue();
//                        dispatcher.TryEnqueue([]()
//                            {
//                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
//                                impl->AnchorButton_Click(nullptr, nullptr);
//                            });
//                    }
//
//                    logToDebugger("Pipe Handler - Status updated to ANCHOR");
//                }
//                else if (command == "SET_ENGINESAIL") {
//                    if (g_mainWindow) {
//                        auto dispatcher = g_mainWindow.DispatcherQueue();
//                        dispatcher.TryEnqueue([]()
//                            {
//                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
//                                impl->SailAndEngineButton_Click(nullptr, nullptr);
//                            });
//                    }
//
//                    logToDebugger("Pipe Handler - Status updated to ENGINESAIL");
//                }
//                else {
//                    std::string unknown = "UNKNOWN COMMAND";
//                    WriteFile(hPipe, unknown.c_str(), static_cast<DWORD>(unknown.size()), &bytesWritten, NULL);
//                    logToDebugger("Pipe Handler - Unknown command.");
//                }
//
//                FlushFileBuffers(hPipe);
//                DisconnectNamedPipe(hPipe);
//                CloseHandle(hPipe);
//                }).detach(); // 🚀 Launch and detach the lambda thread
//        }
//        else {
//            CloseHandle(hPipe);
//        }
//    }
//
//    g_loggerEvents.LogMessage("thread PipeServerLoopThread ended", 2);
//    logToDebugger("thread PipeServerLoopThread ended");
//}

void PipeServerLoop()
{
    struct VesselState {
        std::wstring latlon = L"39° 53' 34.00\" N   4° 16' 21.89\" E";
        double cog = 120.5;
        double tripdist = 34.261;
        std::string status = "UNKNOWN";
    };

    g_loggerEvents.LogMessage("thread PipeServerLoopThread started", 2);
    logToDebugger("thread PipeServerLoopThread started");

    while (!g_shouldStopThreads)
    {
        HANDLE hPipe = CreateNamedPipe(
            L"\\\\.\\pipe\\NMEA_PIPE",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            512, 512,
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            logToDebugger("CreateNamedPipe failed. Error: ", GetLastError());
            break;
        }

        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            // Capture state per client
            VesselState state;

            // Lambda to handle the client
            std::thread([hPipe, state]() mutable {
                char buffer[128] = {};
                DWORD bytesRead = 0;

                BOOL result = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                if (!result || bytesRead == 0) {
                    DisconnectNamedPipe(hPipe);
                    CloseHandle(hPipe);
                    return;
                }

                std::string command(buffer, bytesRead);
                command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());
                command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());

                logToDebugger("Pipe Command received: ", command);
                DWORD bytesWritten = 0;

                if (command == "GET_LATLON") {
                    try {
                        std::wstring positionText = L"";
                        //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                        //std::string reply = converter.to_bytes(state.latlon);

                        if (GPSData.GetDataReliability())
                        {
                            auto latDMS = ConvertToDMS(GPSData.GetLatitude(), true);
                            auto lonDMS = ConvertToDMS(GPSData.GetLongitude(), false);

                            positionText = latDMS + L"   " + lonDMS;
                        }

                        std::string reply = WStringToUtf8(positionText);

                        WriteFile(hPipe, reply.c_str(), static_cast<DWORD>(reply.size()), &bytesWritten, NULL);
                    }
                    catch (const std::exception& e) {
                        logToDebugger("Pipe Handler - UTF-8 encoding error: ", e.what());
                    }
                }
                else if (command == "GET_COG") {
                    double COGData = 0.0;

                    if (GPSData.GetDataReliability()) {
                        COGData = GPSData.GetCog();
                    }

                    std::string reply = FormatDoubleForGermanLocale(COGData, 4);
                    WriteFile(hPipe, reply.c_str(), static_cast<DWORD>(reply.size()), &bytesWritten, NULL);
                }
                else if (command == "GET_TRIPDIST") {
                    double tripDistData = 0.0;

                    if (GPSData.GetDataReliability()) {
                        tripDistData = GPSData.GetTripDist();
                    }

                    std::string reply = FormatDoubleForGermanLocale(tripDistData, 4);
                    WriteFile(hPipe, reply.c_str(), static_cast<DWORD>(reply.size()), &bytesWritten, NULL);
                }
                else if (command == "SET_ENGINE") {
                    if (g_mainWindow) {
                        auto dispatcher = g_mainWindow.DispatcherQueue();
                        dispatcher.TryEnqueue([]()
                            {
                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                                impl->EngineButton_Click(nullptr, nullptr);
                            });
                    }

                    logToDebugger("Pipe Handler - Status updated to ENGINE");
                }
                else if (command == "SET_SAIL") {
                    if (g_mainWindow) {
                        auto dispatcher = g_mainWindow.DispatcherQueue();
                        dispatcher.TryEnqueue([]()
                            {
                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                                impl->SailButton_Click(nullptr, nullptr);
                            });
                    }

                    logToDebugger("Pipe Handler - Status updated to SAIL");
                }
                else if (command == "SET_DOCKED") {
                    if (g_mainWindow) {
                        auto dispatcher = g_mainWindow.DispatcherQueue();
                        dispatcher.TryEnqueue([]()
                            {
                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                                impl->DockedButton_Click(nullptr, nullptr);
                            });
                    }

                    logToDebugger("Pipe Handler - Status updated to DOCKED");
                }
                else if (command == "SET_ANCHOR") {
                    if (g_mainWindow) {
                        auto dispatcher = g_mainWindow.DispatcherQueue();
                        dispatcher.TryEnqueue([]()
                            {
                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                                impl->AnchorButton_Click(nullptr, nullptr);
                            });
                    }

                    logToDebugger("Pipe Handler - Status updated to ANCHOR");
                }
                else if (command == "SET_ENGINESAIL") {
                    if (g_mainWindow) {
                        auto dispatcher = g_mainWindow.DispatcherQueue();
                        dispatcher.TryEnqueue([]()
                            {
                                auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                                impl->SailAndEngineButton_Click(nullptr, nullptr);
                            });
                    }

                    logToDebugger("Pipe Handler - Status updated to ENGINESAIL");
                }
                else {
                    std::string unknown = "UNKNOWN COMMAND";
                    WriteFile(hPipe, unknown.c_str(), static_cast<DWORD>(unknown.size()), &bytesWritten, NULL);
                    logToDebugger("Pipe Handler - Unknown command.");
                }

                FlushFileBuffers(hPipe);
                DisconnectNamedPipe(hPipe);
                CloseHandle(hPipe);
                }).detach(); // 🚀 Launch and detach the lambda thread
        }
        else {
            CloseHandle(hPipe);
        }
    }

    g_loggerEvents.LogMessage("thread PipeServerLoopThread ended", 2);
    logToDebugger("thread PipeServerLoopThread ended");
}

void appPulse()
{
    std::unique_lock<std::mutex> lock(g_thread_mutex);

    g_loggerEvents.LogMessage("thread appPulse started", 2);
    logToDebugger("thread appPulse started");

    while (!g_shouldStopThreads)
    {
        lock.unlock();
        GPSData.CheckDataAge();
        GPSData.CalculateAndUpdateDistance();
        lock.lock();

        //std::this_thread::sleep_for(std::chrono::milliseconds(APP_PULSE_WAITING_MSEC));
        g_cv.wait_for(lock, std::chrono::milliseconds(APP_PULSE_WAITING_MSEC), [] {
            return g_shouldStopThreads.load();
        });
    }

    g_loggerEvents.LogMessage("thread appPulse ended", 2);
    logToDebugger("thread appPulse ended");
}

void queueProcessing()
{
    std::unique_lock<std::mutex> lock(g_thread_mutex);

    g_loggerEvents.LogMessage("thread queueProcessing started", 2);
    logToDebugger("thread queueProcessing started");

    while (!g_shouldStopThreads)
    {
        lock.unlock();

        while (isServerAvailable(g_config.GetServerName())) {
            std::string message;
            if (reportQueue.try_pop(message)) {
                //g_loggerEvents.LogMessage("send_posreport: queued message sent", 1);
                send_udp_message(g_config.GetServerName(), g_config.GetServerPort(), message);
            }
            else {
                //g_loggerEvents.LogMessage("Nothing to pop", 1);
                break; // Stop processing if the queue is empty
            }

            if (g_mainWindow) {
                auto dispatcher = g_mainWindow.DispatcherQueue();
                dispatcher.TryEnqueue([]()
                    {
                        auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                        impl->SetFooterCounter(reportQueue.size());
                    });
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        lock.lock();

        //std::this_thread::sleep_for(std::chrono::milliseconds(QUEUE_PROCESSING_WAITING_MSEC));
        g_cv.wait_for(lock, std::chrono::milliseconds(QUEUE_PROCESSING_WAITING_MSEC), [] {
            return g_shouldStopThreads.load();
        });
    }

    g_loggerEvents.LogMessage("thread queueProcessing ended", 2);
    logToDebugger("thread queueProcessing ended");
}

void send_posreport()
{
    std::unique_lock<std::mutex> lock(g_thread_mutex);

    g_loggerEvents.LogMessage("thread send_posreport started", 2);
    logToDebugger("thread send_posreport started", true);

    while (!g_shouldStopThreads)
    {
        lock.unlock();

        if (g_config.GetSendPosReports() == 1 && GPSData.GetDataReliability()) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);

            std::tm tm_buf;
            localtime_s(&tm_buf, &now_time);

            std::ostringstream oss;
            oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

            std::string mysql_formatted_timestamp = oss.str();

            gpsData::main_data_struct main_data;
            main_data = GPSData.GetData();

            std::ostringstream message;

            message << "1;" << g_config.GetApiKey() << ";"
                << (g_config.GetCallSign().empty() ? "-" : g_config.GetCallSign()) << ";"
                << (g_config.GetShipName().empty() ? "-" : g_config.GetShipName()) << ";"
                << mysql_formatted_timestamp << ";"
                << std::setprecision(4) << main_data.cog << ";"
                << std::setprecision(4) << main_data.sog << ";"
                << std::setprecision(12) << main_data.lat << ";"
                << std::setprecision(12) << main_data.lon << ";"
                << (g_config.GetShipDest().empty() ? "-" : g_config.GetShipDest()) << ";"
                << g_config.GetShipStatus();

            reportQueue.push(message.str());

            if (g_mainWindow) {
                auto dispatcher = g_mainWindow.DispatcherQueue();
                dispatcher.TryEnqueue([]()
                    {
                        auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                        impl->SetFooterCounter(reportQueue.size());
                    });
            }

            //g_loggerEvents.LogMessage("send_posreport: data is reliable, report pushed into queue - " + GPSData.GetAllMainData(), 2);
        }
        else {
            if (g_config.GetSendPosReports() == 0) {
                //g_loggerEvents.LogMessage("send_posreport: sending reports is deactivated  in configfile - " + GPSData.GetAllMainData(), 1);
            }
            else {
                //g_loggerEvents.LogMessage("send_posreport: data is not reliable, no report sent - " + GPSData.GetAllMainData(), 1);
            }
        }

        lock.lock();

        //std::this_thread::sleep_for(std::chrono::milliseconds(POS_REPORT_WAITING_MSEC));
        g_cv.wait_for(lock, std::chrono::milliseconds(POS_REPORT_WAITING_MSEC), [] {
            return g_shouldStopThreads.load();
        });
    }

    g_loggerEvents.LogMessage("thread send_posreport ended", 2);
    logToDebugger("thread send_posreport ended");
}

void sendHeartBeat()
{
    std::unique_lock<std::mutex> lock(g_thread_mutex);

    g_loggerEvents.LogMessage("thread sendHeartBeat started", Logger::LOG_INFO);
    logToDebugger("thread sendHeartBeat started");

    while (!g_shouldStopThreads)
    {
        lock.unlock();

        std::ostringstream message;
        message << "10;" << g_config.GetApiKey();
        send_udp_message(g_config.GetServerName(), g_config.GetServerPort(), message.str());

        lock.lock();

        //std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_FREQUENCY_MSECONDS));
        g_cv.wait_for(lock, std::chrono::milliseconds(HEARTBEAT_FREQUENCY_MSECONDS), [] {
            return g_shouldStopThreads.load();
        });
    }

    g_loggerEvents.LogMessage("thread sendHeartBeat ended", Logger::LOG_INFO);
    logToDebugger("thread sendHeartBeat ended");
}

void checkRemoteServer()
{
    std::unique_lock<std::mutex> lock(g_thread_mutex);
    bool serverAvailability = false;

    g_loggerEvents.LogMessage("thread checkRemoteServer started", Logger::LOG_INFO);
    logToDebugger("thread checkRemoteServer started");

    while (!g_shouldStopThreads)
    {
        lock.unlock();

        serverAvailability = isServerAvailable(g_config.GetServerName());

        //logToDebugger("Serveravailability: ", serverAvailability);

        if (serverAvailability && g_mainWindow) {
            auto dispatcher = g_mainWindow.DispatcherQueue();
            dispatcher.TryEnqueue([]()
                {
                    auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                    impl->SetSRVIndicatorGreen();
                });
        }
        else {
            auto dispatcher = g_mainWindow.DispatcherQueue();
            dispatcher.TryEnqueue([]()
                {
                    auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
                    impl->SetSRVIndicatorRed();
                });
        }

        lock.lock();

        //std::this_thread::sleep_for(std::chrono::milliseconds(FREQUENCY_CHECK_SERVER));
        g_cv.wait_for(lock, std::chrono::milliseconds(FREQUENCY_CHECK_SERVER), [] {
            return g_shouldStopThreads.load();
        });
    }

    g_loggerEvents.LogMessage("thread checkRemoteServer ended", Logger::LOG_INFO);
    logToDebugger("thread checkRemoteServer ended");
}

void ListenUDP()
{
    g_loggerEvents.LogMessage("thread ListenUDP started", Logger::LOG_INFO);
    logToDebugger("thread ListenUDP started");

    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == SOCKET_ERROR) {
        std::cerr << "Error opening socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return;
    }

    u_long mode = 1;  // 1 to enable non-blocking socket
    if (ioctlsocket(sockfd, FIONBIO, &mode) != 0) {
        std::cerr << "ioctlsocket() failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return;
    }

    char buffer[BUFFER_LENGTH];
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    auto dispatcher = g_mainWindow.DispatcherQueue();

    while (!g_shouldStopThreads)
    {
        int bytesReceived = recvfrom(sockfd, buffer, BUFFER_LENGTH - 1, 0, (sockaddr*)&clientAddr, &clientAddrSize);

        if (bytesReceived == SOCKET_ERROR) {

            int error = WSAGetLastError();

            if (error == WSAEWOULDBLOCK) {
                // No data available, do nothing or perform idle tasks
                std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Sleep to prevent tight loop that consumes CPU resources
                continue;
            }
            else {
                std::cerr << "recvfrom() failed with error: " << error << std::endl;
                break;
            }
        }

        if (bytesReceived >= 0 && bytesReceived < BUFFER_LENGTH) {
            buffer[bytesReceived] = '\0';
        }
        else if (bytesReceived >= BUFFER_LENGTH) {
            buffer[BUFFER_LENGTH - 1] = '\0'; // guarantee null
        }
        else {
            buffer[0] = '\0';
        }

        g_loggerGPS.LogMessage(RemoveSpecialCharacters(buffer));

        std::string safe(buffer, bytesReceived);
        dispatcher.TryEnqueue([safe]() {
            auto impl = winrt::get_self<winrt::NMEA_Relay_NT::implementation::MainWindow>(g_mainWindow);
            impl->SetStatusBarFooterText(winrt::to_hstring(safe));
            });

        try {

            if (strlen(buffer) > 2) {

                if (strncmp(buffer, "20", 2) == 0) {

                    // A message starting with 20 is a posreport sentence from another boat
                    // and shall thus be translated to an AIS message sent to OpenCPN
                    //DecodeAndSendForeignPOSReport(buffer);
                }
                else {
                    nmea::sentence nmea_sentence(buffer);

                    if (isInSet(gpsMessages, nmea_sentence.type())) {

                        if (nmea_sentence.type() == "GGA") {

                            nmea::gga gga(nmea_sentence);

                            if (gga.latitude.exists() && gga.longitude.exists() && gga.hdop.exists() && gga.altitude.exists() && gga.fix.exists()) {
                                GPSData.SetData(gga.latitude.get(), gga.longitude.get(), gga.hdop.get(), gga.utc.get(), gga.altitude.get(), gga.fix.get());
                            }

                            if (gga.satellite_count.exists()) {
                                GPSData.SetSatelliteCount(gga.satellite_count.get());
                            }

                            GPSData.ReportSentenceComplete();

                        }
                        else if (nmea_sentence.type() == "GLL") {

                            nmea::gll gll(nmea_sentence);

                            if (gll.status.exists()) {

                                if (gll.status.get() == nmea::status::ACTIVE) {

                                    if (gll.latitude.exists() && gll.longitude.exists() && gll.utc.exists()) {
                                        GPSData.SetData(gll.latitude.get(), gll.longitude.get(), GPSData.GetHdop(), gll.utc.get(), GPSData.GetAlt(), GPSData.GetFix());
                                    }

                                    GPSData.ReportSentenceComplete();
                                }
                            }

                        }
                        else if (nmea_sentence.type() == "VTG") {

                            nmea::vtg vtg(nmea_sentence);

                            if (vtg.speed_knots.exists()) {
                                GPSData.SetSog(vtg.speed_knots.get());
                            }

                            if (vtg.track_angle_true.exists()) {
                                GPSData.SetCog(vtg.track_angle_true.get());
                            }

                        }
                        else if (nmea_sentence.type() == "ZDA") {

                            nmea::zda zda(nmea_sentence);

                            if (zda.utc.exists()) {
                                GPSData.SetUtc(zda.utc.get());
                            }

                        }
                        else if (nmea_sentence.type() == "RMC") {

                            nmea::rmc rmc(nmea_sentence);

                            if (rmc.speed.exists()) {
                                GPSData.SetSog(rmc.speed.get());
                            }

                            if (rmc.track_angle.exists()) {
                                GPSData.SetCog(rmc.track_angle.get());
                            }

                            GPSData.ReportSentenceComplete();
                        }
                    }
                    else if (isInSet(aisMessages, nmea_sentence.type()))
                    {
                        //OutputDebugStringMod(" - AIS Message", true);
                    }
                    else
                    {
                        //OutputDebugStringMod(" - non-conformative message", true);
                    }
                }
            }
        }
        catch (const std::runtime_error& e) {
            // Handle a specific type of exception (runtime_error)
            logToDebugger("ListenUDP - Runtime error: ", e.what());

        }
        catch (const std::exception& e) {
            // Handle all other standard exceptions
            logToDebugger("ListenUDP - An error occurred: ", e.what());

        }
        catch (...) {
            // Handle any other type of exception not caught by previous handlers
            logToDebugger("ListenUDP - An unknown error occurred.");
        }
    }

    closesocket(sockfd);
    WSACleanup();

    g_loggerEvents.LogMessage("thread ListenUDP ended", Logger::LOG_INFO);
    logToDebugger("thread ListenUDP ended");
}

void tcp_keep_connection_alive()
{
    std::unique_lock<std::mutex> lock(g_thread_mutex);

    SOCKET sock = INVALID_SOCKET;
    auto last_restart_time = std::chrono::steady_clock::now();
    std::string recv_buffer;  // Buffer to accumulate partial messages

    while (!g_shouldStopThreads)
    {
        lock.unlock();

        // Check if it's time to restart the connection
        auto now = std::chrono::steady_clock::now();
        if (sock != INVALID_SOCKET && std::chrono::duration_cast<std::chrono::milliseconds>(now - last_restart_time).count() >= TCP_RECONNECT_INTERVAL_MS) {
            logToDebugger("Restarting connection...");
            closesocket(sock);
            sock = INVALID_SOCKET;
            last_restart_time = now;  // Reset the timer
            recv_buffer.clear();  // Clear buffer to avoid corrupted messages
        }

        // Connect if not connected
        if (sock == INVALID_SOCKET) {
            sock = tcp_connect_to_server(g_config.GetServerName().c_str(), PORT_TCP_SERVER);
            if (sock != INVALID_SOCKET) {
                last_restart_time = std::chrono::steady_clock::now(); // Reset restart timer on successful connection
            }
        }

        if (sock != INVALID_SOCKET) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock, &read_fds);

            struct timeval timeout;
            timeout.tv_sec = 2;  // 2-second timeout
            timeout.tv_usec = 0;

            int select_result = select(sock + 1, &read_fds, NULL, NULL, &timeout);
            if (select_result > 0) {
                char buffer[1024];  // Temporary buffer for received data
                int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);

                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0'; // Null-terminate received data
                    recv_buffer.append(buffer);    // Append to buffer

                    // Process complete messages delimited by '\n'
                    size_t pos;
                    while ((pos = recv_buffer.find('\n')) != std::string::npos) {
                        std::string message = recv_buffer.substr(0, pos);
                        recv_buffer.erase(0, pos + 1); // Remove processed message

                        // **Remove trailing `\n`, `\r`, or spaces**
                        while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == ' ')) {
                            message.pop_back();
                        }

                        // ✅ Message is now clean and ready for processing
                        logToDebugger(message.c_str());
                        g_loggerEvents.LogMessage("Received foreign POS report through TCP channel: " + message, 2);
                        sendAISMessagePosition(message);
                    }
                }
                else if (bytes_received == 0 || bytes_received == SOCKET_ERROR) {
                    logToDebugger("Connection closed or error, reconnecting...");
                    closesocket(sock);
                    sock = INVALID_SOCKET;
                    recv_buffer.clear();  // Clear buffer on disconnect
                }
            }
            else if (select_result == 0) {
                // No data received, continue waiting
            }
            else {
                logToDebugger("Select error, reconnecting...");
                closesocket(sock);
                sock = INVALID_SOCKET;
            }
        }

        lock.lock();

        // If not connected, wait and try to reconnect
        if (sock == INVALID_SOCKET) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Wait 1 second before retrying
            g_cv.wait_for(lock, std::chrono::milliseconds(1000), [] {
                return g_shouldStopThreads.load();
            });
        }
    }
}

// Function to initialize Winsock
void InitializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        exit(1);
    }
}

// Native COM-Interface für HWND-Zugriff
struct __declspec(uuid("6d5140c1-7436-11ce-8034-00aa006009fa"))
    IWindowNative : IUnknown
{
    virtual HRESULT __stdcall get_WindowHandle(HWND* hwnd) = 0;
};

// Hilfsfunktion: Zentriert ein WinUI 3 Window auf dem Bildschirm
void CenterWindow(winrt::Microsoft::UI::Xaml::Window const& window)
{
    HWND hwnd{};
    winrt::com_ptr<IWindowNative> windowNative = window.as<IWindowNative>();
    windowNative->get_WindowHandle(&hwnd);

    RECT rect{};
    GetWindowRect(hwnd, &rect);

    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    SetWindowPos(hwnd, nullptr, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void send_udp_message(const std::string& server, int port, const std::string& message) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return;
    }

    struct addrinfo hints = { 0 }, * res = NULL;

    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // Use UDP
    hints.ai_protocol = IPPROTO_UDP; // UDP protocol

    std::string port_str = std::to_string(port);
    if (getaddrinfo(server.c_str(), port_str.c_str(), &hints, &res) != 0) {
        std::cerr << "getaddrinfo failed.\n";
        WSACleanup();
        return;
    }

    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        freeaddrinfo(res);
        WSACleanup();
        return;
    }

    int send_result = sendto(sock, message.c_str(), static_cast<int>(message.size()), 0, res->ai_addr, static_cast<int>(res->ai_addrlen));
    if (send_result == SOCKET_ERROR) {
        std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
        g_loggerEvents.LogMessage("send_udp_message failed: " + std::to_string(WSAGetLastError()), 1);
    }
    else {
        std::cout << "Message sent successfully to " << server << std::endl;
    }

    freeaddrinfo(res);
    closesocket(sock);
    WSACleanup();
}

std::string RemoveSpecialCharacters(const std::string& input) {
    std::string result;
    for (char ch : input) {
        if (isprint(static_cast<unsigned char>(ch))) {  // Keep only printable characters
            result += ch;
        }
    }
    return result;
}

std::wstring RemoveSpecialCharacters(const std::wstring& input) {
    std::wstring result;
    for (char ch : input) {
        if (isprint(static_cast<unsigned char>(ch))) {  // Keep only printable characters
            result += ch;
        }
    }
    return result;
}

void DecodeAndSendForeignPOSReport(const char* buffer)
{
    //OutputDebugStringMod(buffer, true);
}

// Function to check if a string is in a set
bool isInSet(const std::set<std::string>& list, const std::string& messageType) {
    return list.find(messageType) != list.end();
}

std::wstring ConvertToDMS(double decimalDegrees, bool isLatitude) {
    wchar_t direction;
    if (isLatitude) {
        direction = (decimalDegrees >= 0) ? L'N' : L'S';
    }
    else {
        direction = (decimalDegrees >= 0) ? L'E' : L'W';
    }

    decimalDegrees = fabs(decimalDegrees);
    int degrees = static_cast<int>(decimalDegrees);
    double minutesFull = (decimalDegrees - degrees) * 60.0;
    int minutes = static_cast<int>(minutesFull);
    double seconds = (minutesFull - minutes) * 60.0;

    wchar_t buffer[50];
    // Correct format to avoid leading zeros in minutes and seconds
    swprintf_s(buffer, 50, L"%d\u00B0 %d' %05.2f\" %c", degrees, minutes, seconds, direction);

    return std::wstring(buffer);
}

SOCKET tcp_connect_to_server(const char* server_hostname, int server_port) {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, hints;

    logToDebugger("tcp_connect_to_server entered");

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        logToDebugger("WSAStartup failed");
        return INVALID_SOCKET;
    }

    // Set up the hints structure
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // Use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP socket
    hints.ai_protocol = IPPROTO_TCP;    // TCP protocol

    // Convert the integer port to a string
    std::string port_str = std::to_string(server_port);

    // Resolve the server address and port
    int addrinfo_result = getaddrinfo(server_hostname, port_str.c_str(), &hints, &result);
    if (addrinfo_result != 0) {
        logToDebugger("getaddrinfo failed: ", addrinfo_result);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Create a socket for connecting to the server
    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        logToDebugger("Socket creation failed: ", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Connect to the server
    if (connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        logToDebugger("Connection failed: ", WSAGetLastError());
        closesocket(ConnectSocket);
        freeaddrinfo(result);
        WSACleanup();
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);  // Free the addrinfo structure after the connection is made
    logToDebugger("Connected to server, sending API key: ", g_config.GetApiKey());

    // Send API key to server
    if (send(ConnectSocket, g_config.GetApiKey().c_str(), g_config.GetApiKey().length(), 0) == SOCKET_ERROR) {
        logToDebugger("Failed to send API key: ", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    logToDebugger("tcp_connect_to_server finished ok ");

    return ConnectSocket;
}

int sendAISMessagePosition(std::string foreignPosReport) {
    AisProcessor AIS;

    logToDebugger("sendAISMessagePosition entered");

    // Variables to hold the parsed values
    int mmsi;
    std::string callSign;
    std::string vesselName;
    std::string timestamp;
    double course;
    float speed;
    double lat;
    double lon;
    std::string destination;
    int shipStatus;

    // Tokenizing the message
    std::stringstream ss(foreignPosReport);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, ';')) {
        tokens.push_back(token);
    }

    // Interpret the tokens (assuming the structure of the message is known)
    if (tokens.size() == 10) {
        mmsi = std::stoi(tokens[0]);
        callSign = tokens[1];
        vesselName = tokens[2];
        timestamp = tokens[3];  // This is the datetime, but it's unused in your example.
        course = std::stod(tokens[4]);  // Course in degrees
        speed = std::stof(tokens[5]);   // Speed over ground in knots
        lat = std::stod(tokens[6]);     // Latitude
        lon = std::stod(tokens[7]);     // Longitude
        destination = tokens[8];        // Destination
        shipStatus = std::stoi(tokens[9]); // Ship status
    }
    else {
        logToDebugger("sendAISMessagePosition - Error: Unexpected message format.");
        logToDebugger("--------------------------------------------");
        logToDebugger(foreignPosReport.c_str());
        logToDebugger("--------------------------------------------");
        return -1;
    }

    std::string binaryMessage = AIS.createAisMessageType18(mmsi, speed, true, lon, lat, course, course, 50);
    std::string encodedAisCharacters = AIS.EncodeToAISCharacters(binaryMessage);
    std::string nmeaSentence = AIS.CreateNMEASentence(encodedAisCharacters);
    //logToDebugger(nmeaSentence);
    send_udp_message(g_config.GetOpenCpnServer().c_str(), g_config.GetOpenCpnPort(), nmeaSentence);

    binaryMessage = AIS.createAisMessageType24A(mmsi, vesselName);
    encodedAisCharacters = AIS.EncodeToAISCharacters(binaryMessage);
    nmeaSentence = AIS.CreateNMEASentence(encodedAisCharacters);
    send_udp_message(g_config.GetOpenCpnServer().c_str(), g_config.GetOpenCpnPort(), nmeaSentence);

    return 0;
}

std::string WStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return {};

    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};

    std::string result(size - 1, '\0');  // -1 to exclude null terminator
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

// Converts degrees to radians
inline double DegreesToRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

// Calculates distance between two coordinates in nautical miles
double CalculateDistanceNm(double lat1, double lon1, double lat2, double lon2) {
    // Convert degrees to radians
    double lat1_rad = DegreesToRadians(lat1);
    double lon1_rad = DegreesToRadians(lon1);
    double lat2_rad = DegreesToRadians(lat2);
    double lon2_rad = DegreesToRadians(lon2);

    // Haversine formula
    double delta_lat = lat2_rad - lat1_rad;
    double delta_lon = lon2_rad - lon1_rad;

    double a = std::pow(std::sin(delta_lat / 2), 2) +
        std::cos(lat1_rad) * std::cos(lat2_rad) *
        std::pow(std::sin(delta_lon / 2), 2);

    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    return kEarthRadiusNm * c;
}

std::string FormatDoubleForGermanLocale(double value, int precision = 4) {
    std::ostringstream stream;
    try {
        stream.imbue(std::locale("de_DE.UTF-8"));
    }
    catch (...) {
        stream.imbue(std::locale::classic());
    }
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::wstring FormatDoubleForGermanLocaleW(double value, int precision = 4) {
    std::wostringstream stream;
    try {
        stream.imbue(std::locale("de_DE.UTF-8"));  // try German locale
    }
    catch (...) {
        stream.imbue(std::locale::classic());      // fallback to default locale
    }
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

void StopThreads()
{
    g_shouldStopThreads = true;
    g_cv.notify_all();
}