#pragma once

#include "AnchorWatchWindow.g.h"
#include "Globals.h"
#include <winrt/Windows.System.Threading.h>
#include <winrt/Microsoft.UI.Windowing.h>

namespace winrt::NMEA_Relay_NT::implementation
{
    struct AnchorWatchWindow : AnchorWatchWindowT<AnchorWatchWindow>
    {
        double m_anchorLat = 0.0;
        double m_anchorLon = 0.0;
        double m_anchorLatCos = 0.0;
        double m_prevLat = 0.0;
        double m_prevLon = 0.0;
        double m_scale = 1.0; // store scale for consistent drawing
        int m_currentRadiusMeters = 10;
        bool m_canvasInitialized = false;
        double m_prevScreenX = 0;
        double m_prevScreenY = 0;
        bool m_isClosed = false;
        bool m_alarmActive = false;
        bool m_alarmArmed = false;
        winrt::Windows::System::Threading::ThreadPoolTimer m_alarmTimer{ nullptr };
        winrt::Windows::System::Threading::ThreadPoolTimer m_refreshTimer{ nullptr };
        winrt::Microsoft::UI::Xaml::Shapes::Ellipse m_latestPositionDot{ nullptr };

        void StartAlarm();
        void StopAlarm();
        void OnAlarmBlinkTick(winrt::Windows::System::Threading::ThreadPoolTimer const&);
        void PlayAlarmSound();

        std::thread m_alarmSoundThread;
        std::atomic<bool> m_alarmSoundThreadRunning = false;

        AnchorWatchWindow()
        {
            InitializeComponent();

            HWND hwnd = GetWindowHandle();

            // Apply style changes immediately
            ::SetWindowPos(hwnd, nullptr, 100, 100, 1200, 900,
                SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

            // Initialize anchor position from GPSData
            m_anchorLat = GPSData.GetLatitude();
            m_anchorLon = GPSData.GetLongitude();
            m_anchorLatCos = cos(m_anchorLat * M_PI / 180);

            // Initial previous point is also the anchor
            m_prevLat = m_anchorLat;
            m_prevLon = m_anchorLon;

            m_gpsHistory = GPSData.GetGpsHistory();

            AnchorCanvas().LayoutUpdated([this](auto&&, auto&&)
                {
                    if (!m_canvasInitialized && AnchorCanvas().ActualWidth() > 0 && AnchorCanvas().ActualHeight() > 0)
                    {
                        DrawAnchorAndRadius(m_currentRadiusMeters);
                        m_canvasInitialized = true;
                    }
                });

            StartGpsRefreshTimer();

            RadiusInput().Text(std::to_wstring(m_currentRadiusMeters));

            this->Closed([this](auto&&, auto&&)
                {
                    m_isClosed = true;
                    if (m_refreshTimer) m_refreshTimer.Cancel();
                    if (m_alarmTimer) m_alarmTimer.Cancel();
                    g_anchorWatchWindow = nullptr;
            });

            AnchorCanvas().SizeChanged({ this, &AnchorWatchWindow::OnCanvasSizeChanged });
            UpdateAnchorLatLonText();
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void RadiusInput_TextChanged(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& e);

        void OnCanvasSizeChanged(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& args);

        //winrt::Windows::System::Threading::ThreadPoolTimer m_refreshTimer{ nullptr };
        void StartGpsRefreshTimer();
        void OnGpsRefreshTimerTick(winrt::Windows::System::Threading::ThreadPoolTimer const& timer);

        void ResetAnchorPosition_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void MoveAnchorWithOffset_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void MoveAnchorWithOffsetMedian_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void MoveAnchorWithOffsetFromAnchor_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void AlarmToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void SetFooterText(winrt::hstring const& text);

        std::pair<double, double> CalculateGpsHistoryCenter() const;

        void ResetAnchorPositionMedian_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void TestAlarm_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void AnchorLatLonText_Tapped(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&);

        void DrawAnchorAndRadius(int radiusMeters);
        void DrawAnchorVisuals();
        void DrawGpsPosition(double lat, double lon);

        std::vector<std::pair<double, double>> m_gpsHistory; // lat, lon pairs
        void RedrawAllGpsPoints();
        std::pair<double, double> ConvertLatLonToScreen(double lat, double lon);

        void UpdateAnchorLatLonText();
        HWND GetWindowHandle();
    };
}

namespace winrt::NMEA_Relay_NT::factory_implementation
{
    struct AnchorWatchWindow : AnchorWatchWindowT<AnchorWatchWindow, implementation::AnchorWatchWindow>
    {
    };
}
