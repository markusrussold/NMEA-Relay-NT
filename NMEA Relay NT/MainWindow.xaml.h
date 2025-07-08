#pragma once

#include "MainWindow.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::NMEA_Relay_NT::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        HWND GetWindowHandle();
    };
}

namespace winrt::NMEA_Relay_NT::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
