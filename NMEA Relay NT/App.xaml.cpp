#include "pch.h"
#include "Globals.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include <thread>
#include "HelperFunctions.h"
#include "Constants.h"
#include <winrt/Windows.Globalization.h>

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
        // Setze die App-Sprache (z. B. Deutsch/Österreich)
        winrt::Windows::Globalization::ApplicationLanguages::PrimaryLanguageOverride(L"de-AT");

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

        if (g_config.GetTripDistance() > 0) {
            GPSData.SetTripDist(g_config.GetTripDistance());
        }

        posReportThread = std::thread(send_posreport);
        heartbeatThread = std::thread(sendHeartBeat);
        checkServerThread = std::thread(checkRemoteServer);
        udpListenThread = std::thread(ListenUDP);
        tcpKeepAliveThread = std::thread(tcp_keep_connection_alive);
        appPulseThread = std::thread(appPulse);
        queueProcessingThread = std::thread(queueProcessing);
        PipeServerLoopThread = std::thread(PipeServerLoop);

        // Shutdown-Hook
        myWindow.Closed([this](auto&&, auto&&) {
            g_loggerEvents.LogMessage("Beginne Threads zu beenden", 2);
            TerminateThread(PipeServerLoopThread.native_handle(), 0);
            g_loggerEvents.LogMessage("Beendigung Threads zweite Stufe", 2);
            StopThreads();
            g_loggerEvents.LogMessage("Alle Threads beendet", 2);

            if (posReportThread.joinable()) posReportThread.join();
            if (heartbeatThread.joinable()) heartbeatThread.join();
            if (checkServerThread.joinable()) checkServerThread.join();
            if (udpListenThread.joinable()) udpListenThread.join();
            if (tcpKeepAliveThread.joinable()) tcpKeepAliveThread.join();
            if (appPulseThread.joinable()) appPulseThread.join();
            if (queueProcessingThread.joinable()) queueProcessingThread.join();
            //if (PipeServerLoopThread.joinable()) PipeServerLoopThread.join();

            g_loggerEvents.LogMessage("App shutdown via Window.Closed", Logger::LOG_INFO);
            logToDebugger("App shutdown via Window.Closed");
            });
    }

}
