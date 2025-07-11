#include "pch.h"
#include "Globals.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include <thread>
#include "HelperFunctions.h"
#include "Constants.h"

extern void send_posreport();
extern void sendHeartBeat();
extern void checkRemoteServer();
extern void ListenUDP();
extern void tcp_keep_connection_alive();

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::NMEA_Relay_NT::implementation
{
    App::App()
    {
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
            {
                if (IsDebuggerPresent())
                {
                    auto errorMessage = e.Message();
                    __debugbreak();
                }
            });
#endif

    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        winrt::NMEA_Relay_NT::MainWindow myWindow = winrt::NMEA_Relay_NT::MainWindow();
        myWindow.Activate();
        g_mainWindow = myWindow;

        CenterWindow(myWindow);

        g_config.Load();
        g_config.ApplyToWindow();

        InitializeWinsock();

        g_loggerEvents.SetLogLevel(Logger::LOG_INFO);

        logToDebugger("MaxDataAgeSeconds config: ", g_config.GetGpsMaxDataAgeSeconds());
        if (g_config.GetGpsMaxDataAgeSeconds() >= 5 && g_config.GetGpsMaxDataAgeSeconds() <= MAX_DATA_AGE_SECONDS) {
            GPSData.SetMaxDataAgeSeconds(g_config.GetGpsMaxDataAgeSeconds());
        }
        logToDebugger("MaxDataAgeSeconds: ", g_config.GetGpsMaxDataAgeSeconds());

        posReportThread = std::thread(send_posreport);
        heartbeatThread = std::thread(sendHeartBeat);
        checkServerThread = std::thread(checkRemoteServer);
        udpListenThread = std::thread(ListenUDP);
        tcpKeepAliveThread = std::thread(tcp_keep_connection_alive);
        appPulseThread = std::thread(appPulse);
        queueProcessingThread = std::thread(queueProcessing);

        // Shutdown-Hook
        myWindow.Closed([this](auto&&, auto&&) {
            g_shouldStopThreads = true;

            if (posReportThread.joinable()) posReportThread.join();
            if (heartbeatThread.joinable()) heartbeatThread.join();
            if (checkServerThread.joinable()) checkServerThread.join();
            if (udpListenThread.joinable()) udpListenThread.join();
            if (tcpKeepAliveThread.joinable()) tcpKeepAliveThread.join();
            if (appPulseThread.joinable()) appPulseThread.join();
            if (queueProcessingThread.joinable()) queueProcessingThread.join();

            g_loggerEvents.LogMessage("App shutdown via Window.Closed", Logger::LOG_INFO);
            logToDebugger("App shutdown via Window.Closed");
            });
    }
}
