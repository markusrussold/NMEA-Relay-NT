#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <Windows.h>
#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

struct __declspec(uuid("6d5140c1-7436-11ce-8034-00aa006009fa")) IWindowNative : IUnknown
{
    virtual HRESULT __stdcall get_WindowHandle(HWND* hwnd) = 0;
};

namespace winrt::NMEA_Relay_NT::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        HWND hwnd = GetWindowHandle();
        ::SetWindowPos(hwnd, HWND_TOP, 100, 100, 800, 500, SWP_NOZORDER);
    }

    HWND GetWindowHandle()
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
}
