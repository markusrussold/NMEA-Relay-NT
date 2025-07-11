﻿#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <cstdlib>
#include <Windows.h>
#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include "resource.h"
#include <thread>
#include <string>
#include <winrt/Windows.Storage.h>
#include <shellapi.h> 
#include "HelperFunctions.h"
#include "Globals.h"
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <sstream>
#include <iomanip>
#include <winrt/Windows.UI.Popups.h>
#include "version.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI;
using namespace winrt::Windows::UI::Popups;

namespace winrt::NMEA_Relay_NT::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        HWND hwnd = GetWindowHandle();

        // Load the icon resource:
        HICON hIcon = static_cast<HICON>(LoadImageW(
            GetModuleHandleW(nullptr),
            MAKEINTRESOURCEW(IDI_ICON1),
            IMAGE_ICON,
            0, 0, LR_DEFAULTSIZE | LR_SHARED));

        // Apply it:
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        // Get current window style
        LONG style = ::GetWindowLong(hwnd, GWL_STYLE);

        // Remove the resizable border (WS_SIZEBOX) and maximize box (WS_MAXIMIZEBOX)
        style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);

        // Apply new style
        ::SetWindowLong(hwnd, GWL_STYLE, style);

        // Apply style changes immediately
        ::SetWindowPos(hwnd, nullptr, 100, 100, 1100, 600,
            SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

        PositionBar().Background(SolidColorBrush(Colors::LightCoral()));
        PositionBarText().Text(L"Keine gültigen Positionsdaten");
        ShipRotation().Angle(0);
        SogBar().Value(0.0);
        SogValueText().Text(L"");
        FooterCounter().Text(L"0");
        StatusBarFooterText().Text(L"");
        SRVIndicator().Background(SolidColorBrush(Colors::Red()));

        BoatNameTextBox().Text(L"Sea Pearl Super");
        DestinationTextBox().Text(L"Weit Weit Weg");
        CallsignTextBox().Text(L"NotUsed");
        ServerNameTextBox().Text(L"mrussold.com");
        ServerPortTextBox().Text(L"1234");
        KeyTextBox().Text(L"seapearl4519");
        OpenCPNTextBox().Text(L"192.168.0.220");
        OpenCPNPortTextBox().Text(L"2947");

        AnchorButton().Background(SolidColorBrush(Colors::LightGreen()));
    }

    HWND MainWindow::GetWindowHandle()
    {
        HWND hwnd{};

        auto windowNative = this->try_as<::IWindowNative>();
        if (windowNative)
        {
            windowNative->get_WindowHandle(&hwnd);
        }
        else
        {
            throw std::runtime_error("IWindowNative cast failed");
        }

        return hwnd;
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::OpenConfigPath_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        using namespace winrt::Windows::Storage;

        // Hole Roaming-Pfad
        auto folder = ApplicationData::Current().RoamingFolder();
        auto path = folder.Path();

        // Öffne im Explorer (ShellExecute erwartet wchar_t*)
        ShellExecuteW(
            nullptr,
            L"open",
            path.c_str(),
            nullptr,
            nullptr,
            SW_SHOWDEFAULT
        );

        // Feedback im StatusBarFooter
        StatusBarFooterText().Text(L"Konfig-Pfad geöffnet: " + path);
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::Version_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        using namespace winrt::Microsoft::UI::Xaml::Controls;

        // Puffer für den formatierten String
        wchar_t buffer[256];
        swprintf_s(
            buffer,
            L"NMEA Relay NT\n"
            L"Version %d.%d.%d (Build %d)\n"
            L"Copyright © Markus Russold\n"
            L"All rights reserved.",
            PROJECT_VERSION_MAJOR,
            PROJECT_VERSION_MINOR,
            PROJECT_VERSION_PATCH,
            PROJECT_VERSION_BUILD
        );

        ContentDialog dialog;
        dialog.Title(box_value(L"App Info"));
        dialog.Content(box_value(buffer));
        dialog.CloseButtonText(L"OK");

        dialog.XamlRoot(PositionBar().XamlRoot());
        dialog.ShowAsync();
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::ShowGpsDebugInfo_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        using namespace winrt::Microsoft::UI::Xaml::Controls;

        ContentDialog dialog;
        dialog.Title(box_value(L"GPS Debug Info"));

        // Get the C++ string from your GPSData:
        std::string gpsInfo = GPSData.GetAllMainData();

        // Convert it to hstring:
        winrt::hstring gpsText = winrt::to_hstring(gpsInfo);

        TextBlock textBlock;
        textBlock.Text(gpsText);
        textBlock.TextWrapping(winrt::Microsoft::UI::Xaml::TextWrapping::Wrap);

        dialog.Content(textBlock);
        dialog.CloseButtonText(L"OK");

        // Fix the XamlRoot!
        dialog.XamlRoot(this->Content().XamlRoot());

        dialog.ShowAsync();
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::Exit_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        g_mainWindow.Close();
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::PositionBar_Tapped(
        winrt::Windows::Foundation::IInspectable const& /*sender*/,
        winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& /*e*/)
    {
        using namespace winrt::Windows::ApplicationModel::DataTransfer;

        auto dataPackage = DataPackage();
        dataPackage.SetText(PositionBarText().Text());

        Clipboard::SetContent(dataPackage);
        Clipboard::Flush();
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::StatusBarFooter_Tapped(
        winrt::Windows::Foundation::IInspectable const& /*sender*/,
        winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& /*e*/)
    {
        using namespace winrt::Windows::ApplicationModel::DataTransfer;

        auto dataPackage = DataPackage();
        dataPackage.SetText(StatusBarFooterText().Text());

        Clipboard::SetContent(dataPackage);
        Clipboard::Flush();
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::MessageBox_KeyDown(
        winrt::Windows::Foundation::IInspectable const& /*sender*/,
        winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e)
    {
        if (e.Key() == winrt::Windows::System::VirtualKey::Enter)
        {
            auto message = MessageBox().Text();
            OutputDebugStringW((L"Nachricht: " + std::wstring(message) + L"\n").c_str());
            MessageBox().Text(L"");
            e.Handled(true);
        }
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::SailButton_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        ClearStatusHighlights();
        SailButton().Background(SolidColorBrush(Colors::LightGreen()));
        g_loggerEvents.LogMessage("Status wurde auf Segeln gesetzt", Logger::LOG_INFO);
        logToDebugger("Segeln wurde ausgewäht.");

        try {
            g_config.SetShipStatus(8);
        }
        catch (...) {
            // Handle parse error if needed
        }
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::EngineButton_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        ClearStatusHighlights();
        EngineButton().Background(SolidColorBrush(Colors::LightGreen()));
        g_loggerEvents.LogMessage("Status wurde auf Motor gesetzt", Logger::LOG_INFO);
        logToDebugger("Motorantrieb wurde ausgewäht.");

        try {
            g_config.SetShipStatus(0);
        }
        catch (...) {
            // Handle parse error if needed
        }
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::SailAndEngineButton_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        ClearStatusHighlights();
        SailAndEngineButton().Background(SolidColorBrush(Colors::LightGreen()));
        g_loggerEvents.LogMessage("Status wurde auf Segeln und Motor gesetzt", Logger::LOG_INFO);
        logToDebugger("Segel und Motor wurde ausgewäht.");

        try {
            g_config.SetShipStatus(20);
        }
        catch (...) {
            // Handle parse error if needed
        }
    }

    void MainWindow::AnchorButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        ClearStatusHighlights();
        AnchorButton().Background(SolidColorBrush(Colors::LightGreen()));
        g_loggerEvents.LogMessage("Status wurde auf Ankern gesetzt", Logger::LOG_INFO);
        logToDebugger("Ankern wurde ausgewäht.");

        try {
            g_config.SetShipStatus(1);
        }
        catch (...) {
            // Handle parse error if needed
        }
    }

    void MainWindow::DockedButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        ClearStatusHighlights();
        DockedButton().Background(SolidColorBrush(Colors::LightGreen()));
        g_loggerEvents.LogMessage("Status wurde auf Festgemacht gesetzt", Logger::LOG_INFO);
        logToDebugger("Festgemacht wurde ausgewäht.");

        try {
            g_config.SetShipStatus(5);
        }
        catch (...) {
            // Handle parse error if needed
        }
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::SetButtonStatus(int status)
    {
        // Zuerst alle zurücksetzen (optional)
        SailButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::White()));
        EngineButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::White()));
        AnchorButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::White()));
        DockedButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::White()));
        SailAndEngineButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::White()));

        // Setze den gewünschten Button auf grün
        switch (status)
        {
        case 0:
            EngineButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::LightGreen()));
            break;
        case 1:
            AnchorButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::LightGreen()));
            break;
        case 5:
            DockedButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::LightGreen()));
            break;
        case 8:
            SailButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::LightGreen()));
            break;
        case 20:
            SailAndEngineButton().Background(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::LightGreen()));
            break;
        default:
            // Unknown status → nichts machen oder Fehler loggen
            break;
        }
    }

    void MainWindow::TestButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        ShipRotation().Angle(180);
        SetSRVIndicatorRed();
    }

    void MainWindow::ClearStatusHighlights()
    {
        SailButton().Background(SolidColorBrush(Colors::White()));
        EngineButton().Background(SolidColorBrush(Colors::White()));
        SailAndEngineButton().Background(SolidColorBrush(Colors::White()));
        AnchorButton().Background(SolidColorBrush(Colors::White()));
        DockedButton().Background(SolidColorBrush(Colors::White()));
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::ConfigTextBox_TextChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&)
    {
        auto textBox = sender.as<winrt::Microsoft::UI::Xaml::Controls::TextBox>();
        auto tag = textBox.Tag().as<winrt::hstring>();
        auto text = textBox.Text();

        std::wstring msg = L"Changed [" + std::wstring(tag) + L"] = " + std::wstring(text) + L"\n";
        OutputDebugStringW(msg.c_str());

        // --- Update g_config ---
        std::string tagStr = winrt::to_string(tag);
        std::string valueStr = winrt::to_string(text);

        if (tagStr == "BoatName") {
            g_config.SetShipName(valueStr);
        }
        else if (tagStr == "Destination") {
            g_config.SetShipDest(valueStr);
        }
        else if (tagStr == "Callsign") {
            g_config.SetCallSign(valueStr);
        }
        else if (tagStr == "ServerName") {
            g_config.SetServerName(valueStr);
        }
        else if (tagStr == "ServerPort") {
            try {
                g_config.SetServerPort(std::stoi(valueStr));
            }
            catch (...) {
                // Handle parse error if needed
            }
        }
        else if (tagStr == "Key") {
            g_config.SetApiKey(valueStr);
        }
        else if (tagStr == "OpenCPN") {
            g_config.SetOpenCpnServer(valueStr);
        }
        else if (tagStr == "OpenCPNPort") {
            try {
                g_config.SetOpenCpnPort(std::stoi(valueStr));
            }
            catch (...) {
                // Handle parse error if needed
            }
        }
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::SetSRVIndicatorGreen()
    {
        SRVIndicator().Background(
            winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(
                winrt::Windows::UI::Colors::Green()
            )
        );
    }

    void MainWindow::SetSRVIndicatorRed()
    {
        SRVIndicator().Background(
            winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(
                winrt::Windows::UI::Colors::Red()
            )
        );
    }

    // Setters
    void MainWindow::SetBoatName(winrt::hstring const& value) {
        BoatNameTextBox().Text(value);
    }

    void MainWindow::SetDestination(winrt::hstring const& value) {
        DestinationTextBox().Text(value);
    }

    void MainWindow::SetCallsign(winrt::hstring const& value) {
        CallsignTextBox().Text(value);
    }

    void MainWindow::SetServerName(winrt::hstring const& value) {
        ServerNameTextBox().Text(value);
    }

    void MainWindow::SetServerPort(winrt::hstring const& value) {
        ServerPortTextBox().Text(value);
    }

    void MainWindow::SetKey(winrt::hstring const& value) {
        KeyTextBox().Text(value);
    }

    void MainWindow::SetOpenCPN(winrt::hstring const& value) {
        OpenCPNTextBox().Text(value);
    }

    void MainWindow::SetOpenCPNPort(winrt::hstring const& value) {
        OpenCPNPortTextBox().Text(value);
    }

    void MainWindow::SetStatusBarFooterText(const winrt::hstring& text)
    {
        std::wstring tmp(text);
        std::replace(tmp.begin(), tmp.end(), L'\n', L' ');
        StatusBarFooterText().Text(winrt::hstring(tmp));
    }

    void MainWindow::SetSOG(double sogValue)
    {
        // Setze den Balken-Wert
        SogBar().Value(sogValue);

        // Formatiere den Text schön mit 2 Nachkommastellen + Einheit
        std::wstringstream stream;
        stream << std::fixed << std::setprecision(2) << sogValue << L" kn";

        SogValueText().Text(winrt::hstring(stream.str()));
    }

    void MainWindow::SetCOG(double cogValue)
    {
        if (cogValue < 0) {
            ShipIcon().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Collapsed);

            COGValueText().Text(winrt::hstring(L""));
        }
        else {
            ShipIcon().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Visible);
            ShipRotation().Angle(cogValue);

            std::wstringstream ss;
            ss << std::fixed << std::setprecision(0) << cogValue << L"°";
            COGValueText().Text(winrt::hstring(ss.str()));
        }
    }

    void MainWindow::SetDataReliability(bool dataReliabilityValue)
    {
        if (dataReliabilityValue)
        {
            PositionBar().Background(SolidColorBrush(Colors::LightGreen()));
        }
        else
        {
            PositionBar().Background(SolidColorBrush(Colors::LightCoral()));
            PositionBarText().Text(L"Keine gültigen Positionsdaten");
        }
    }

    void MainWindow::SetLatLon(double lat, double lon)
    {
        if (GPSData.GetDataReliability())
        {
            auto latDMS = ConvertToDMS(lat, true);
            auto lonDMS = ConvertToDMS(lon, false);

            std::wstring positionText = latDMS + L"   " + lonDMS;

            PositionBarText().Text(winrt::hstring{ positionText });
        }
        else {
            PositionBarText().Text(L"Keine gültigen Positionsdaten");
        }
    }


#include <chrono>

    void MainWindow::SetUtc(double utc)
    {
        // Zerlege die NMEA UTC hhmmss.sss
        int hours = static_cast<int>(utc) / 10000;
        int minutes = (static_cast<int>(utc) % 10000) / 100;
        int seconds = static_cast<int>(utc) % 100;

        // Hole DataAge direkt von GPSData (deine globale Instanz)
        int dataAgeSeconds = GPSData.GetDataAge();

        wchar_t buffer[60];
        swprintf_s(
            buffer,
            60,
            L"UTC: %02d:%02d:%02d (%d s)",
            hours,
            minutes,
            seconds,
            dataAgeSeconds
        );

        UTCText().Text(winrt::hstring(buffer));
    }

    void MainWindow::SetDataAge(int dataAge)
    {
        // Hole UTC aus GPSData (dein Getter liefert double, z. B. 141516.0)
        double utc = GPSData.GetUtc();

        // Zerlege NMEA UTC hhmmss.sss
        int hours = static_cast<int>(utc) / 10000;
        int minutes = (static_cast<int>(utc) % 10000) / 100;
        int seconds = static_cast<int>(utc) % 100;

        // Ausgabe mit übergebenem DataAge
        wchar_t buffer[60];
        swprintf_s(
            buffer,
            60,
            L"UTC: %02d:%02d:%02d (%d s)",
            hours,
            minutes,
            seconds,
            dataAge
        );

        // Zeige an
        UTCText().Text(winrt::hstring(buffer));
    }

    void MainWindow::SetFooterCounter(int value)
    {
        std::wstringstream ss;
        ss << value;
        FooterCounter().Text(winrt::hstring(ss.str()));
    }
}


