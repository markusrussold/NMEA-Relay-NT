#pragma once

#include "MainWindow.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include "Constants.h"

namespace winrt::NMEA_Relay_NT::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void OpenConfigPath_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void Version_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void ShowGpsDebugInfo_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void Exit_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void PositionBar_Tapped(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);

        void StatusBarFooter_Tapped(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&);

        void MessageBox_KeyDown(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);

        void SailButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void EngineButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void SailAndEngineButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void AnchorButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void DockedButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void TestButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void ClearStatusHighlights();

        void SetButtonStatus(int status);

        void ConfigTextBox_TextChanged(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& e);

        void SetSRVIndicatorGreen();
        void SetSRVIndicatorRed();

        // Setters
        void SetBoatName(winrt::hstring const& value);
        void SetDestination(winrt::hstring const& value);
        void SetCallsign(winrt::hstring const& value);
        void SetServerName(winrt::hstring const& value);
        void SetServerPort(winrt::hstring const& value);
        void SetKey(winrt::hstring const& value);
        void SetOpenCPN(winrt::hstring const& value);
        void SetOpenCPNPort(winrt::hstring const& value);
        void SetFooterCounter(int value);

        void MainWindow::SetStatusBarFooterText(const winrt::hstring& text);

        void SetSOG(double sogValue);
        void SetCOG(double cogValue);
        void SetDataReliability(bool dataReliabilityValue);
        void SetLatLon(double lat, double lon);
        void SetUtc(double utc);
        void MainWindow::SetDataAge(int dataAge);

    private:
        HWND GetWindowHandle();
    };
}

namespace winrt::NMEA_Relay_NT::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}

std::string getRoamingAppDataFolder();