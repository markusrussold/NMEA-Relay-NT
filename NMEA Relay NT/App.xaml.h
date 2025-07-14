#pragma once

#include "App.xaml.g.h"
#include <thread>
#include <winrt/Windows.ApplicationModel.h>

namespace winrt::NMEA_Relay_NT::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };

        std::thread posReportThread;
        std::thread heartbeatThread;
        std::thread checkServerThread;
        std::thread udpListenThread;
        std::thread tcpKeepAliveThread;
        std::thread appPulseThread;
        std::thread queueProcessingThread;
        std::thread PipeServerLoopThread;
    };
}
