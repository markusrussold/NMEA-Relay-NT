#pragma once

#define FREQUENCY_CHECK_SERVER 2000 // Number of milliseconds between execution of check-server procedure
#define HEARTBEAT_FREQUENCY_MSECONDS 2000
#define MAX_DATA_AGE_SECONDS 7*24*60*60 // 1 Woche maximum
#define POS_REPORT_WAITING_MSEC 3000
#define PORT 8082
#define BUFFER_LENGTH 1025
#define APP_PULSE_WAITING_MSEC 1000
#define QUEUE_PROCESSING_WAITING_MSEC 50
#define TCP_RECONNECT_INTERVAL_MS 5000
#define PORT_TCP_SERVER 8020 // Remote port of Server for TCP Connection

constexpr double kEarthRadiusNm = 3440.065; // Earth's radius in nautical miles

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif