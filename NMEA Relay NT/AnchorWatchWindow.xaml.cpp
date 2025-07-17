#include "pch.h"
#include "AnchorWatchWindow.xaml.h"
#if __has_include("AnchorWatchWindow.g.cpp")
#include "AnchorWatchWindow.g.cpp"
#endif

#include <algorithm> 
#include "HelperFunctions.h"
#include "Globals.h"
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.System.Threading.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <microsoft.ui.xaml.window.h> // wichtig für IWindowNative

using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace winrt::Windows::System::Threading;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace winrt::NMEA_Relay_NT::implementation
{
    void AnchorWatchWindow::RadiusInput_TextChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&)
    {
        auto text = RadiusInput().Text();
        try
        {
            int radiusMeters = std::stoi(std::wstring(text));
            radiusMeters = std::clamp(radiusMeters, 1, 1000);
            m_currentRadiusMeters = radiusMeters;

            DrawAnchorAndRadius(m_currentRadiusMeters);
        }
        catch (...)
        {
            // Ignore invalid input
        }
    }

    HWND AnchorWatchWindow::GetWindowHandle()
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

    void AnchorWatchWindow::OnCanvasSizeChanged(winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const&)
    {
        DrawAnchorAndRadius(m_currentRadiusMeters);
    }

    void AnchorWatchWindow::DrawAnchorAndRadius(int radiusMeters)
    {
        if (!AnchorCanvas() || AnchorCanvas().ActualWidth() == 0 || AnchorCanvas().ActualHeight() == 0)
            return;

        AnchorCanvas().Children().Clear();

        double canvasHeight = AnchorCanvas().ActualHeight();
        double visualPixelRadius = canvasHeight / 2 * 0.9;
        m_scale = visualPixelRadius / radiusMeters;

        DrawAnchorVisuals();
        RedrawAllGpsPoints();
    }

    void AnchorWatchWindow::DrawAnchorVisuals()
    {
        if (!AnchorCanvas() || AnchorCanvas().ActualWidth() == 0 || AnchorCanvas().ActualHeight() == 0)
            return;

        double canvasWidth = AnchorCanvas().ActualWidth();
        double canvasHeight = AnchorCanvas().ActualHeight();
        double centerX = canvasWidth / 2;
        double centerY = canvasHeight / 2;

        double visualPixelRadius = canvasHeight / 2 * 0.9;

        winrt::Microsoft::UI::Xaml::Shapes::Ellipse circle;
        circle.Width(visualPixelRadius * 2);
        circle.Height(visualPixelRadius * 2);
        circle.Stroke(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Blue()));
        circle.StrokeThickness(2);
        Canvas::SetLeft(circle, centerX - visualPixelRadius);
        Canvas::SetTop(circle, centerY - visualPixelRadius);
        AnchorCanvas().Children().Append(circle);

        winrt::Microsoft::UI::Xaml::Shapes::Ellipse dot;
        dot.Width(10);
        dot.Height(10);
        dot.Fill(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Red()));
        Canvas::SetLeft(dot, centerX - 5);
        Canvas::SetTop(dot, centerY - 5);
        AnchorCanvas().Children().Append(dot);
    }

    void AnchorWatchWindow::DrawGpsPosition(double lat, double lon)
    {
        if (!AnchorCanvas() || AnchorCanvas().ActualWidth() == 0 || AnchorCanvas().ActualHeight() == 0)
            return;

        auto [newX, newY] = ConvertLatLonToScreen(lat, lon);

        if (m_gpsHistory.empty())
        {
            double canvasWidth = AnchorCanvas().ActualWidth();
            double canvasHeight = AnchorCanvas().ActualHeight();
            double centerX = canvasWidth / 2;
            double centerY = canvasHeight / 2;

            winrt::Microsoft::UI::Xaml::Shapes::Line firstLine;
            firstLine.X1(centerX);
            firstLine.Y1(centerY);
            firstLine.X2(newX);
            firstLine.Y2(newY);
            firstLine.Stroke(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Yellow()));
            firstLine.StrokeThickness(2);
            AnchorCanvas().Children().Append(firstLine);

            m_prevScreenX = centerX;
            m_prevScreenY = centerY;
        }
        else
        {
            winrt::Microsoft::UI::Xaml::Shapes::Line line;
            line.X1(m_prevScreenX);
            line.Y1(m_prevScreenY);
            line.X2(newX);
            line.Y2(newY);
            line.Stroke(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Yellow()));
            line.StrokeThickness(2);
            AnchorCanvas().Children().Append(line);
        }

        m_prevScreenX = newX;
        m_prevScreenY = newY;
        m_gpsHistory.emplace_back(lat, lon);

        if (m_latestPositionDot)
        {
            m_latestPositionDot.Visibility(Visibility::Collapsed);
        }

        m_latestPositionDot = winrt::Microsoft::UI::Xaml::Shapes::Ellipse();
        m_latestPositionDot.Width(10);
        m_latestPositionDot.Height(10);
        m_latestPositionDot.Fill(SolidColorBrush(Windows::UI::Colors::LightGreen()));
        Canvas::SetLeft(m_latestPositionDot, newX - 5);
        Canvas::SetTop(m_latestPositionDot, newY - 5);
        AnchorCanvas().Children().Append(m_latestPositionDot);
    }

    void AnchorWatchWindow::RedrawAllGpsPoints()
    {
        if (!AnchorCanvas() || AnchorCanvas().ActualWidth() == 0 || AnchorCanvas().ActualHeight() == 0)
            return;

        if (m_gpsHistory.empty())
            return;

        bool first = true;
        double prevX = 0;
        double prevY = 0;

        for (const auto& [lat, lon] : m_gpsHistory)
        {
            auto [newX, newY] = ConvertLatLonToScreen(lat, lon);

            if (first)
            {
                first = false;
            }
            else
            {
                winrt::Microsoft::UI::Xaml::Shapes::Line line;
                line.X1(prevX);
                line.Y1(prevY);
                line.X2(newX);
                line.Y2(newY);
                line.Stroke(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Yellow()));
                line.StrokeThickness(2);
                AnchorCanvas().Children().Append(line);
            }

            prevX = newX;
            prevY = newY;
        }

        m_prevScreenX = prevX;
        m_prevScreenY = prevY;
    }


    void AnchorWatchWindow::StartAlarm()
    {
        m_alarmActive = true;

        PlayAlarmSound();

        m_alarmTimer = ThreadPoolTimer::CreatePeriodicTimer(
            { this, &AnchorWatchWindow::OnAlarmBlinkTick },
            std::chrono::milliseconds(500) // blink every 500ms
        );
    }

    void AnchorWatchWindow::StopAlarm()
    {
        m_alarmActive = false;
        m_alarmSoundThreadRunning = false;

        if (m_alarmSoundThread.joinable())
        {
            m_alarmSoundThread.join();
        }

        if (m_alarmTimer) m_alarmTimer.Cancel();

        if (m_isClosed) return;

        if (DispatcherQueue())
        {
            DispatcherQueue().TryEnqueue([this]()
                {
                    if (MainGrid() && SecondGrid() && AnchorCanvas())
                    {
                        MainGrid().Background(SolidColorBrush(Windows::UI::Colors::Black()));
                        SecondGrid().Background(SolidColorBrush(Windows::UI::Colors::Black()));
                        AnchorCanvas().Background(SolidColorBrush(Windows::UI::Colors::Black()));
                    };
                });
        }
    }

    void AnchorWatchWindow::OnAlarmBlinkTick(ThreadPoolTimer const&)
    {
        if (m_isClosed) return;

        try {
            if (DispatcherQueue())
            {
                DispatcherQueue().TryEnqueue([this]()
                    {
                        static bool isRed = false;
                        isRed = !isRed;
                        if (MainGrid() && SecondGrid() && AnchorCanvas())
                        {
                            MainGrid().Background(SolidColorBrush(isRed ? Windows::UI::Colors::Red() : Windows::UI::Colors::Black()));
                            SecondGrid().Background(SolidColorBrush(isRed ? Windows::UI::Colors::Red() : Windows::UI::Colors::Black()));
                            AnchorCanvas().Background(SolidColorBrush(isRed ? Windows::UI::Colors::Red() : Windows::UI::Colors::Black()));
                        };
                    });
            }
        }
        catch (...) {
            g_loggerEvents.LogMessage("Exception in OnAlarmBlinkTick");
        }
    }

    void AnchorWatchWindow::PlayAlarmSound()
    {
        if (m_alarmSoundThreadRunning) return;  // already running

        m_alarmSoundThreadRunning = true;
        m_alarmSoundThread = std::thread([this]()
            {
                try {
                    while (m_alarmActive && m_alarmSoundThreadRunning)
                    {
                        Beep(1500, 200);   // Low tone
                        if (!m_alarmActive || !m_alarmSoundThreadRunning) break;
                        Beep(1000, 200);  // High tone
                        if (!m_alarmActive || !m_alarmSoundThreadRunning) break;
                        Beep(800, 200);   // Low tone
                    }
                }
                catch (...) {
                    g_loggerEvents.LogMessage("Exception in PlayAlarmSound");
                }
            });
    }

    std::pair<double, double> AnchorWatchWindow::ConvertLatLonToScreen(double lat, double lon)
    {
        double deltaLat = (lat - m_anchorLat) * 111000;
        double deltaLon = (lon - m_anchorLon) * 111000 * m_anchorLatCos;

        double canvasWidth = AnchorCanvas().ActualWidth();
        double canvasHeight = AnchorCanvas().ActualHeight();
        double centerX = canvasWidth / 2;
        double centerY = canvasHeight / 2;

        double screenX = centerX + deltaLon * m_scale;
        double screenY = centerY - deltaLat * m_scale;

        return { screenX, screenY };
    }

    int32_t AnchorWatchWindow::MyProperty() { throw hresult_not_implemented(); }
    void AnchorWatchWindow::MyProperty(int32_t) { throw hresult_not_implemented(); }

    void AnchorWatchWindow::StartGpsRefreshTimer()
    {
        using namespace winrt::Windows::Foundation;
        using namespace winrt::Windows::System::Threading;

        m_refreshTimer = ThreadPoolTimer::CreatePeriodicTimer(
            { this, &AnchorWatchWindow::OnGpsRefreshTimerTick },
            std::chrono::seconds(1));
    }

    void AnchorWatchWindow::OnGpsRefreshTimerTick(ThreadPoolTimer const&)
    {
        if (m_isClosed) return;

        try {
            if (auto mainWindow = g_mainWindowWeakRef.get())
            {
                if (GPSData.GetDataReliability())
                {
                    double currentLat = GPSData.GetLatitude();
                    double currentLon = GPSData.GetLongitude();

                    double distanceToOriginNm = CalculateDistanceNm(m_anchorLat, m_anchorLon, currentLat, currentLon);
                    double distanceToOriginMeters = distanceToOriginNm * 1852;

                    std::wstringstream ss;
                    ss << std::fixed << std::setprecision(1) << distanceToOriginMeters;
                    auto distanceText = winrt::hstring(ss.str()) + L" m";

                    if (m_isClosed) return;

                    if (DispatcherQueue())
                    {
                        DispatcherQueue().TryEnqueue([this, currentLat, currentLon, distanceText]()
                            {
                                if (m_isClosed)
                                    return;

                                if (DistanceToOriginTextBox())
                                {
                                    DistanceToOriginTextBox().Text(distanceText);
                                }

                                DrawGpsPosition(currentLat, currentLon);
                            });
                    }

                    if (distanceToOriginMeters > m_currentRadiusMeters)
                    {
                        if (m_alarmArmed) {
                            if (!m_alarmActive)
                            {
                                StartAlarm();
                            }
                            else
                            {
                                PlayAlarmSound();  // keep playing sound
                            }
                        }

                        SetFooterText(L"Aktuelle Position zu weit vom Anker entfernt!!");
                    }
                    else
                    {
                        if (m_alarmActive)
                        {
                            StopAlarm();
                        }

                        SetFooterText(L"");
                    }
                }
                else {
                    if (m_alarmArmed) {
                        if (!m_alarmActive)
                        {
                            StartAlarm();
                        }
                        else
                        {
                            PlayAlarmSound();  // keep playing sound
                        }
                    }

                    SetFooterText(L"Vorsicht, aktuell keine gültigen Positionsdaten verfügbar!");
                }
            }
            else {
                if (m_alarmArmed) {
                    if (!m_alarmActive)
                    {
                        StartAlarm();
                    }
                    else
                    {
                        PlayAlarmSound();  // keep playing sound
                    }
                }

                SetFooterText(L"Hauptfenster wurde geschlossen und es gibt keine Positionsdaten mehr!");
            }
        }
        catch (...) {
            g_loggerEvents.LogMessage("Exception in OnGpsRefreshTimerTick");
        }
    }

    void AnchorWatchWindow::UpdateAnchorLatLonText() {
        auto latDMS = ConvertToDMS(m_anchorLat, true);
        auto lonDMS = ConvertToDMS(m_anchorLon, false);

        AnchorLatLonText().Text(latDMS + L"   " + lonDMS);
    }

    void AnchorWatchWindow::ResetAnchorPosition_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        m_anchorLat = GPSData.GetLatitude();
        m_anchorLon = GPSData.GetLongitude();
        m_anchorLatCos = cos(m_anchorLat * M_PI / 180);

        DrawAnchorAndRadius(m_currentRadiusMeters);
        UpdateAnchorLatLonText();
    }

    void AnchorWatchWindow::MoveAnchorWithOffset_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        try
        {
            double directionDeg = std::stod(std::wstring(OffsetBearingInput().Text()));
            double distanceMeters = std::stod(std::wstring(OffsetDistanceInput().Text()));

            // Convert to radians
            double directionRad = directionDeg * M_PI / 180.0;
            double earthRadiusMeters = 6371000.0;

            double deltaLat = (distanceMeters * cos(directionRad)) / earthRadiusMeters;
            double deltaLon = (distanceMeters * sin(directionRad)) / (earthRadiusMeters * cos(m_anchorLat * M_PI / 180.0));

            // Convert to degrees
            deltaLat = deltaLat * 180.0 / M_PI;
            deltaLon = deltaLon * 180.0 / M_PI;

            // Apply offset
            m_anchorLat = GPSData.GetLatitude() + deltaLat;
            m_anchorLon = GPSData.GetLongitude() + deltaLon;
            m_anchorLatCos = cos(m_anchorLat * M_PI / 180.0);

            DrawAnchorAndRadius(m_currentRadiusMeters);
            UpdateAnchorLatLonText();
        }
        catch (...)
        {
            logToDebugger("Invalid input in offset direction or distance.");
        }
    }

    void AnchorWatchWindow::MoveAnchorWithOffsetMedian_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        try
        {
            double directionDeg = std::stod(std::wstring(OffsetBearingInput().Text()));
            double distanceMeters = std::stod(std::wstring(OffsetDistanceInput().Text()));

            // Convert to radians
            double directionRad = directionDeg * M_PI / 180.0;
            double earthRadiusMeters = 6371000.0;

            double deltaLat = (distanceMeters * cos(directionRad)) / earthRadiusMeters;
            double deltaLon = (distanceMeters * sin(directionRad)) / (earthRadiusMeters * cos(m_anchorLat * M_PI / 180.0));

            // Convert to degrees
            deltaLat = deltaLat * 180.0 / M_PI;
            deltaLon = deltaLon * 180.0 / M_PI;

            // Apply offset
            auto [centerLat, centerLon] = CalculateGpsHistoryCenter();
            m_anchorLat = centerLat + deltaLat;
            m_anchorLon = centerLon + deltaLon;
            m_anchorLatCos = cos(m_anchorLat * M_PI / 180.0);

            // Clear current drawing and redraw
            //m_gpsHistory.clear();
            //m_prevScreenX = 0.0;
            //m_prevScreenY = 0.0;

            DrawAnchorAndRadius(m_currentRadiusMeters);
            UpdateAnchorLatLonText();
        }
        catch (...)
        {
            logToDebugger("Invalid input in offset direction or distance.");
        }
    }

    void AnchorWatchWindow::MoveAnchorWithOffsetFromAnchor_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        try
        {
            double directionDeg = std::stod(std::wstring(OffsetBearingInput().Text()));
            double distanceMeters = std::stod(std::wstring(OffsetDistanceInput().Text()));

            // Convert to radians
            double directionRad = directionDeg * M_PI / 180.0;
            double earthRadiusMeters = 6371000.0;

            double deltaLat = (distanceMeters * cos(directionRad)) / earthRadiusMeters;
            double deltaLon = (distanceMeters * sin(directionRad)) / (earthRadiusMeters * cos(m_anchorLat * M_PI / 180.0));

            // Convert to degrees
            deltaLat = deltaLat * 180.0 / M_PI;
            deltaLon = deltaLon * 180.0 / M_PI;

            // Apply offset
            m_anchorLat += deltaLat;
            m_anchorLon += deltaLon;
            m_anchorLatCos = cos(m_anchorLat * M_PI / 180.0);

            DrawAnchorAndRadius(m_currentRadiusMeters);
            UpdateAnchorLatLonText();
        }
        catch (...)
        {
            logToDebugger("Invalid input in offset direction or distance.");
        }
    }

    void AnchorWatchWindow::AlarmToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        auto toggleSwitch = sender.as<winrt::Microsoft::UI::Xaml::Controls::ToggleSwitch>();
        if (toggleSwitch.IsOn())
        {
            m_alarmArmed = true;
        }
        else
        {
            m_alarmArmed = false;
            if (m_alarmTimer) m_alarmTimer.Cancel();
            StopAlarm();
        }
    }

    void AnchorWatchWindow::SetFooterText(winrt::hstring const& text)
    {
            if (m_isClosed || !DispatcherQueue())
                return;

            if (DispatcherQueue()) {
                DispatcherQueue().TryEnqueue([this, text]()
                    {
                        if (FooterTextBlock())
                        {
                            FooterTextBlock().Text(text);
                        }
                    });
            }
    }

    std::pair<double, double> AnchorWatchWindow::CalculateGpsHistoryCenter() const
    {
        if (m_gpsHistory.empty())
        {
            return { 0.0, 0.0 };  // or consider returning std::optional if no points
        }

        double sumLat = 0.0;
        double sumLon = 0.0;

        for (const auto& [lat, lon] : m_gpsHistory)
        {
            sumLat += lat;
            sumLon += lon;
        }

        double centerLat = sumLat / m_gpsHistory.size();
        double centerLon = sumLon / m_gpsHistory.size();

        return { centerLat, centerLon };
    }

    void AnchorWatchWindow::ResetAnchorPositionMedian_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        auto [centerLat, centerLon] = CalculateGpsHistoryCenter();
        m_anchorLat = centerLat;
        m_anchorLon = centerLon;
        m_anchorLatCos = cos(m_anchorLat * M_PI / 180);

        DrawAnchorAndRadius(m_currentRadiusMeters);
        UpdateAnchorLatLonText();
    }

    void AnchorWatchWindow::TestAlarm_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        Beep(1000, 700);
    }

    void AnchorWatchWindow::ResetTrack_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        m_gpsHistory.clear();

        if (AnchorCanvas())
        {
            DrawAnchorAndRadius(m_currentRadiusMeters);
        }
    }

    void AnchorWatchWindow::AnchorLatLonText_Tapped(
        winrt::Windows::Foundation::IInspectable const& /*sender*/,
        winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& /*e*/)
    {
        using namespace winrt::Windows::ApplicationModel::DataTransfer;

        auto dataPackage = DataPackage();
        dataPackage.SetText(AnchorLatLonText().Text());

        Clipboard::SetContent(dataPackage);
        Clipboard::Flush();
    }

    void AnchorWatchWindow::OnMainGridDoubleTapped(winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e)
    {
        auto point = e.GetPosition(AnchorCanvas());
        double x = point.X;
        double y = point.Y;

        auto [lat, lon] = ConvertScreenToLatLon(x, y);

        m_anchorLat = lat;
        m_anchorLon = lon;
        m_anchorLatCos = cos(m_anchorLat * M_PI / 180.0);

        DrawAnchorAndRadius(m_currentRadiusMeters);
        UpdateAnchorLatLonText();

        SetFooterText(L"Ankerposition auf Doppelklick gesetzt.");
    }

    std::pair<double, double> AnchorWatchWindow::ConvertScreenToLatLon(double screenX, double screenY)
    {
        double canvasWidth = AnchorCanvas().ActualWidth();
        double canvasHeight = AnchorCanvas().ActualHeight();
        double centerX = canvasWidth / 2;
        double centerY = canvasHeight / 2;

        double deltaLonMeters = (screenX - centerX) / m_scale;
        double deltaLatMeters = (centerY - screenY) / m_scale;

        double deltaLat = deltaLatMeters / 111000;
        double deltaLon = deltaLonMeters / (111000 * m_anchorLatCos);

        return { m_anchorLat + deltaLat, m_anchorLon + deltaLon };
    }

}