#include "pch.h"
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

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI;

std::string getRoamingAppDataFolder();

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
        CompassRotation().Angle(0);
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

        ContentDialog dialog;
        dialog.Title(box_value(L"App Info"));
        dialog.Content(box_value(
            L"NMEA Relay NT\n"
            L"Version 1.0.0 (Build 361)\n"
            L"Copyright © Markus Russold\n"
            L"All rights reserved."
        ));
        dialog.CloseButtonText(L"OK");

        // Attach to this window
        dialog.XamlRoot(PositionBar().XamlRoot());

        dialog.ShowAsync();
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::Exit_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        std::exit(0);
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
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::EngineButton_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        ClearStatusHighlights();
        EngineButton().Background(SolidColorBrush(Colors::LightGreen()));
    }

    void winrt::NMEA_Relay_NT::implementation::MainWindow::SailAndEngineButton_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        ClearStatusHighlights();
        SailAndEngineButton().Background(SolidColorBrush(Colors::LightGreen()));
    }

    void MainWindow::AnchorButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        ClearStatusHighlights();
        AnchorButton().Background(SolidColorBrush(Colors::LightGreen()));
    }

    void MainWindow::DockedButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        ClearStatusHighlights();
        DockedButton().Background(SolidColorBrush(Colors::LightGreen()));
    }

    void MainWindow::TestButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        CompassRotation().Angle(90);
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

        StatusBarFooterText().Text(L"Changed [" + std::wstring(tag) + L"] = " + std::wstring(text));
    }
}

std::string getRoamingAppDataFolder()
{
    using namespace winrt::Windows::Storage;
    auto folder = ApplicationData::Current().RoamingFolder();
    return winrt::to_string(folder.Path());
}