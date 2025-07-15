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
        RadiusLabel().Text(L"Aktueller Radius: " + std::to_wstring(radiusMeters) + L" m");

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

    }

    void AnchorWatchWindow::RedrawAllGpsPoints()
    {
        if (!AnchorCanvas() || AnchorCanvas().ActualWidth() == 0 || AnchorCanvas().ActualHeight() == 0)
            return;

        if (m_gpsHistory.empty())
            return;

        double canvasWidth = AnchorCanvas().ActualWidth();
        double canvasHeight = AnchorCanvas().ActualHeight();
        double centerX = canvasWidth / 2;
        double centerY = canvasHeight / 2;

        double prevX = centerX;
        double prevY = centerY;

        for (const auto& [lat, lon] : m_gpsHistory)
        {
            auto [newX, newY] = ConvertLatLonToScreen(lat, lon);

            winrt::Microsoft::UI::Xaml::Shapes::Line line;
            line.X1(prevX);
            line.Y1(prevY);
            line.X2(newX);
            line.Y2(newY);
            line.Stroke(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Yellow()));
            line.StrokeThickness(2);
            AnchorCanvas().Children().Append(line);

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
        if (m_alarmTimer) m_alarmTimer.Cancel();

        if (DispatcherQueue())
        {
            DispatcherQueue().TryEnqueue([this]()
                {
                    if (MainGrid())
                    {
                        MainGrid().Background(SolidColorBrush(Windows::UI::Colors::Black()));
                    };
                });
        }
    }

    void AnchorWatchWindow::OnAlarmBlinkTick(ThreadPoolTimer const&)
    {
        if (DispatcherQueue())
        {
            DispatcherQueue().TryEnqueue([this]()
                {
                    static bool isRed = false;
                    isRed = !isRed;
                    if (MainGrid())
                    {
                        MainGrid().Background(SolidColorBrush(isRed ? Windows::UI::Colors::Red() : Windows::UI::Colors::Black()));
                    };
                });
        }
    }

    void AnchorWatchWindow::PlayAlarmSound()
    {
        Beep(1000, 500);  // Frequency in Hz, duration in ms
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

        double currentLat = GPSData.GetLatitude();
        double currentLon = GPSData.GetLongitude();

        double distanceToOriginNm = CalculateDistanceNm(m_anchorLat, m_anchorLon, currentLat, currentLon);
        double distanceToOriginMeters = distanceToOriginNm * 1852;

        std::wstringstream ss;
        ss << std::fixed << std::setprecision(1) << distanceToOriginMeters;
        auto distanceText = winrt::hstring(ss.str()) + L" m";

        if (DispatcherQueue())
        {
            DispatcherQueue().TryEnqueue([this, currentLat, currentLon, distanceText]()
                {
                    DrawGpsPosition(currentLat, currentLon);
                    DistanceToOriginTextBox().Text(distanceText);
                });
        }

        if (distanceToOriginMeters > m_currentRadiusMeters)
        {
            if (!m_alarmActive)
            {
                StartAlarm();
            }
            else
            {
                PlayAlarmSound();  // keep playing sound
            }
        }
        else
        {
            if (m_alarmActive)
            {
                StopAlarm();
            }
        }
    }


    void AnchorWatchWindow::ResetAnchorPosition_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        m_anchorLat = GPSData.GetLatitude();
        m_anchorLon = GPSData.GetLongitude();
        m_anchorLatCos = cos(m_anchorLat * M_PI / 180);

        m_gpsHistory.clear();
        m_prevScreenX = 0;
        m_prevScreenY = 0;

        DrawAnchorAndRadius(m_currentRadiusMeters);
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
            m_anchorLat += deltaLat;
            m_anchorLon += deltaLon;
            m_anchorLatCos = cos(m_anchorLat * M_PI / 180.0);

            // Clear current drawing and redraw
            m_gpsHistory.clear();
            m_prevScreenX = 0.0;
            m_prevScreenY = 0.0;

            DrawAnchorAndRadius(m_currentRadiusMeters);
        }
        catch (...)
        {
            logToDebugger("Invalid input in offset direction or distance.");
        }
    }

}
