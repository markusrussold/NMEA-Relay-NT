#pragma once

#include "AppConfig.h"        
#include <atomic>
#include "MainWindow.xaml.h"
#include "Logger.h"
#include "HelperFunctions.h"
#include "Constants.h"
#include "gpsData.h"
#include "ReportQueue.h"
#include <condition_variable>
#include <mutex>

// globale Flags
extern std::atomic<bool> g_shouldStopThreads;
extern std::condition_variable g_cv;
extern std::mutex g_thread_mutex;
extern HANDLE g_stopEvent;

// globale Objekte
extern winrt::NMEA_Relay_NT::MainWindow g_mainWindow;
extern Logger g_loggerGPS;
extern Logger g_loggerEvents;

// globale Konfiguration
extern AppConfig g_config;

class gpsData;
extern gpsData GPSData;

class ReportQueue;
extern ReportQueue reportQueue;

extern winrt::NMEA_Relay_NT::AnchorWatchWindow g_anchorWatchWindow;

inline winrt::weak_ref<winrt::NMEA_Relay_NT::implementation::MainWindow> g_mainWindowWeakRef{};
