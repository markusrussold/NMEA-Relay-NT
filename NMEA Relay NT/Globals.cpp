#include "pch.h"
#include "Globals.h"
#include "HelperFunctions.h"
#include "ReportQueue.h"
#include "gpsData.h"
#include <condition_variable>
#include <mutex>

// globale Steuerflagge für Threads
std::atomic<bool> g_shouldStopThreads = false;
std::condition_variable g_cv;
std::mutex g_thread_mutex;

// globale Referenz auf das MainWindow
winrt::NMEA_Relay_NT::MainWindow g_mainWindow{ nullptr };

// globale Logger
Logger g_loggerGPS("GPSInput.txt", false, getRoamingAppDataFolder());
Logger g_loggerEvents("NMEARelayLog.txt", true, getRoamingAppDataFolder());

// globale Konfiguration
AppConfig g_config(getRoamingAppDataFolder() + "\\config.txt");

ReportQueue reportQueue(getRoamingAppDataFolder());
gpsData GPSData;

