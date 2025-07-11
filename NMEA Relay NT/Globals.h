#pragma once

#include "AppConfig.h"        
#include <atomic>
#include "MainWindow.xaml.h"
#include "Logger.h"
#include "HelperFunctions.h"
#include "Constants.h"
#include "gpsData.h"
#include "ReportQueue.h"

// globale Flags
extern std::atomic<bool> g_shouldStopThreads;

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
