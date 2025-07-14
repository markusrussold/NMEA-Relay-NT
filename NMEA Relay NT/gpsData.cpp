#include "pch.h"
#include "gpsData.h"
#include <algorithm>
#include "framework.h"
#include "Observer.h"
#include <iostream>
#include <vector>
#include <utility>
#include <stdexcept>
#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <sstream>
#include <string>
#include <iostream>
#include "Constants.h"
#include "HelperFunctions.h"

#define POS_SLIDING_WINDOW_SIZE 3
#define MAX_HDOP 3.0
#define SMOOTHENING_ON false

std::pair<double, double> gpsData::ProvideSmoothenedPosition(const std::vector<pos_data_struct>& pos_data_list) const {
    if (SMOOTHENING_ON == true) {
        size_t size = pos_data_list.size();

        if (size == 0) {
            return std::make_pair(0.0, 0.0);
        }
        else if (size < POS_SLIDING_WINDOW_SIZE) {
            return std::make_pair(pos_data_list.back().lat, pos_data_list.back().lon);
        }

        double sumLat = 0.0;
        double sumLon = 0.0;
        double weightSum = 0.0;

        // Weights for the most recent, second most recent, and third most recent positions
        const double weights[POS_SLIDING_WINDOW_SIZE] = { 0.7, 0.6, 0.4 };

        // Iterate over the last three positions with corresponding weights
        for (size_t i = 0; i < POS_SLIDING_WINDOW_SIZE; ++i) {
            sumLat += pos_data_list[size - 1 - i].lat * weights[i];
            sumLon += pos_data_list[size - 1 - i].lon * weights[i];
            weightSum += weights[i];
        }

        // Calculate the weighted averages
        double avgLat = sumLat / weightSum;
        double avgLon = sumLon / weightSum;

        return std::make_pair(avgLat, avgLon);
    } else {

        return std::make_pair(pos_data_list.back().lat, pos_data_list.back().lon);
    }
}

void gpsData::Attach(Observer* observer) {
    observers.push_back(observer);
}

void gpsData::Detach(Observer* observer) {
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

void gpsData::NotifyLatLon() {
    for (Observer* observer : observers) {
        observer->UpdateLatLon(main_data.lat, main_data.lon);
    }
}

void gpsData::NotifyHdop() {
    for (Observer* observer : observers) {
        observer->UpdateHdop(main_data.hdop);
    }
}

void gpsData::NotifyUtc() {
    for (Observer* observer : observers) {
        observer->UpdateUtc(main_data.utc);
    }
}

void gpsData::NotifyTripDist() {
    for (Observer* observer : observers) {
        observer->UpdateTripDist(main_data.tripdist);
    }
}

void gpsData::NotifyAlt() {
    for (Observer* observer : observers) {
        observer->UpdateAlt(main_data.alt);
    }
}

void gpsData::NotifyFix() {
    for (Observer* observer : observers) {
        observer->UpdateFix(main_data.fix);
    }
}

void gpsData::NotifySatelliteCount() {
    for (Observer* observer : observers) {
        observer->UpdateSatelliteCount(main_data.satellite_count);
    }
}

void gpsData::NotifyRmcValidity() {
    for (Observer* observer : observers) {
        observer->UpdateRmcValidity(main_data.rmc_valid);
    }
}

void gpsData::NotifySog() {
    for (Observer* observer : observers) {
        observer->UpdateSog(main_data.sog);
    }
}

void gpsData::NotifyCog() {
    for (Observer* observer : observers) {
        observer->UpdateCog(main_data.cog);
    }
}

void gpsData::NotifyDataReliability() {
    for (Observer* observer : observers) {
        observer->UpdateDataReliability(main_data.data_reliability);
    }
}

void gpsData::SetMaxDataAgeSeconds(int MaxDataAgeSecondsInput) {
    MaxDataAgeSeconds = MaxDataAgeSecondsInput;
}

void gpsData::SetData(double newLat, double newLon, double newHdop, double newUtc, double newAlt, uint8_t newFix)
{
    std::lock_guard<std::mutex> lock(mtx);
    pos_data_struct pos_data;

    pos_data.lat = 0.0;
    pos_data.lon = 0.0;
    pos_data.utc = 0.0;

    if (newLat >= -90.0 && newLat <= 90.0) {
        if (newLat != main_data.lat) {
            main_data.lat = newLat;
            pos_data.lat = newLat;
            NotifyLatLon();
        }
        else {
            pos_data.lat = main_data.lat;
        }
    }

    if (newLon >= -180.0 && newLon <= 180.0) {
        if (newLon != main_data.lon) {
            main_data.lon = newLon;
            pos_data.lon = newLon;
            NotifyLatLon();
        }
        else {
            pos_data.lon = main_data.lon;
        }
    }

    if (newHdop != main_data.hdop) {
        main_data.hdop = newHdop;
        NotifyHdop();

        if (main_data.hdop > 3) {
            SetDataReliability(false);
        }
    }

    if (newUtc != main_data.utc) {
        main_data.utc = newUtc;
        pos_data.utc = newUtc;
        NotifyUtc();
    }
    else {
        pos_data.utc = main_data.utc;
    }

    if (newAlt != main_data.alt) {
        main_data.alt = newAlt;
        NotifyAlt();
    }

    if (newFix != main_data.fix) {
        main_data.fix = newFix;
        NotifyFix();

        if (newFix == 0 || newFix == 4 || newFix == 5 || newFix == 6) {
            SetDataReliability(false);
        }
    }

    if (newHdop <= MAX_HDOP && (main_data.fix == 1 || main_data.fix == 2 || main_data.fix == 3 || main_data.fix == 7 || main_data.fix == 8)) {
        pos_data_list.push_back(pos_data);
    }
}

void gpsData::SetHdop(double newHdop) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newHdop != main_data.hdop) {
        main_data.hdop = newHdop;
        NotifyHdop();

        if (main_data.hdop > 3) {
            SetDataReliability(false);
        }
    }
}

void gpsData::SetUtc(double newUtc) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newUtc != main_data.utc) {
        main_data.utc = newUtc;
        NotifyUtc();
    }
}

void gpsData::SetTripDist(double newTripDist) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newTripDist != main_data.tripdist) {
        main_data.tripdist = newTripDist;
        NotifyTripDist();
    }
}

void gpsData::SetAlt(double newAlt) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newAlt != main_data.alt) {
        main_data.alt = newAlt;
        NotifyAlt();
    }
}

void gpsData::SetFix(uint8_t newFix) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newFix != main_data.fix) {
        main_data.fix = newFix;
        NotifyFix();

        if (newFix == 0 || newFix == 4 || newFix == 5 || newFix == 6) {
            SetDataReliability(false);
        }
    }
}

void gpsData::SetSatelliteCount(uint8_t newSatelliteCount) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newSatelliteCount >= 0 && newSatelliteCount <= 100) {
        if (newSatelliteCount != main_data.satellite_count) {
            main_data.satellite_count = newSatelliteCount;
            NotifySatelliteCount();
        }
    }
}

//void gpsData::SetRmcValidity(bool newRmcValidity) {
//    std::lock_guard<std::mutex> lock(mtx);
//    if (newRmcValidity != main_data.rmc_valid) {
//        main_data.rmc_valid = newRmcValidity;
//        NotifyRmcValidity();
//
//        if (!main_data.rmc_valid) {
//            //SetDataReliability(false);
//        }
//    }
//}

void gpsData::SetSog(float newSog) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newSog >= 0) {
        if (newSog != main_data.sog) {
            main_data.sog = newSog;
            NotifySog();
        }
    }
}

void gpsData::SetCog(float newCog) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newCog >= 0) {
        if (newCog != main_data.cog) {
            main_data.cog = newCog;
            NotifyCog();
        }
    }
}

void gpsData::SetDataReliability(bool newData_reliability) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newData_reliability != main_data.data_reliability) {
        main_data.data_reliability = newData_reliability;
        NotifyDataReliability();
    }
}

gpsData::main_data_struct gpsData::GetData()
{
    main_data_struct temp_data = main_data;
    std::lock_guard<std::mutex> lock(mtx);

    try {
        auto [avgLat, avgLon] = ProvideSmoothenedPosition(pos_data_list);
        temp_data.lat = avgLat;
        temp_data.lon = avgLon;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return temp_data;
}

double gpsData::GetLatitude() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.lat;
}

double gpsData::GetLongitude() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.lon;
}

double gpsData::GetHdop() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.hdop;
}

double gpsData::GetUtc() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.utc;
}

double gpsData::GetTripDist() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.tripdist;
}

double gpsData::GetAlt() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.alt;
}

uint8_t gpsData::GetFix() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.fix;
}

uint8_t gpsData::GetSatelliteCount() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.satellite_count;
}

bool gpsData::GetRmcValidity() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.rmc_valid;
}

float gpsData::GetSog() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.sog;
}

float gpsData::GetCog() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.cog;
}

bool gpsData::GetDataReliability() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data.data_reliability;
}

void gpsData::ReportSentenceComplete() {
    if (pos_data_list.size() >= 1 && main_data.hdop < MAX_HDOP && (main_data.fix == 1 || main_data.fix == 2 || main_data.fix == 3 || main_data.fix == 7 || main_data.fix == 8)) {
        int age = CalculateTimeDifferenceInSeconds();
        if (age <= MaxDataAgeSeconds) {
            SetDataReliability(true);
            //logger.LogMessage("Data reliability set to true\n");
        } else {
            //std::ostringstream oss;
            //oss << "Data age exceeded: " << age << " and only " << MaxDataAgeSeconds << " allowed.";
            //std::string result = oss.str();
            //logger.LogMessage(result + "\n");
        }
    }
    else {
        //logger.LogMessage("Data reliability - no change\n");
    }

}

std::string gpsData::GetAllMainData() {
    std::ostringstream oss;
    int age = CalculateTimeDifferenceInSeconds();
    //if (age <= MaxDataAgeSeconds) {

    oss << "age = " << age << " seconds, "
        << "lat = " << main_data.lat << ", "
        << "lon = " << main_data.lon << ", "
        << "hdop = " << main_data.hdop << ", "
        << "utc = " << main_data.utc << ", "
        << "alt = " << main_data.alt << ", "
        << "sog = " << main_data.sog << ", "
        << "cog = " << main_data.cog << ", "
        << "fix = " << static_cast<int>(main_data.fix) << ", "
        << "satellite_count = " << static_cast<int>(main_data.satellite_count) << ", "
        << "rmc_valid = " << (main_data.rmc_valid ? "true" : "false") << ", "
        << "data_reliability = " << (main_data.data_reliability ? "true" : "false");

    return oss.str();
}

double gpsData::GetCurrentUtcTimeAsDouble() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = {};

    // Use gmtime_s instead of gmtime for safer code
    gmtime_s(&now_tm, &now_time);

    // Convert current time to a double in the format HHMMSS.SS
    double currentTime = now_tm.tm_hour * 10000 + now_tm.tm_min * 100 + now_tm.tm_sec;
    return currentTime;
}

int gpsData::GetMaxDataAgeSeconds() const {
    return MaxDataAgeSeconds;
}

int gpsData::CalculateTimeDifferenceInSeconds() {
    double current_utc_time = GetCurrentUtcTimeAsDouble();

    // Extract hours, minutes, and seconds from the GPS UTC time
    int gps_hours = static_cast<int>(main_data.utc / 10000);
    int gps_minutes = static_cast<int>((main_data.utc - gps_hours * 10000) / 100);
    int gps_seconds = static_cast<int>(main_data.utc) % 100;

    // Extract hours, minutes, and seconds from the current UTC time
    int current_hours = static_cast<int>(current_utc_time / 10000);
    int current_minutes = static_cast<int>((current_utc_time - current_hours * 10000) / 100);
    int current_seconds = static_cast<int>(current_utc_time) % 100;

    // Convert both times to seconds since midnight
    int gps_total_seconds = gps_hours * 3600 + gps_minutes * 60 + gps_seconds;
    int current_total_seconds = current_hours * 3600 + current_minutes * 60 + current_seconds;

    // Calculate the difference
    int time_difference = current_total_seconds - gps_total_seconds;

    // Handle the situation where the time crosses midnight
    if (time_difference < 0) {
        time_difference += 24 * 3600; // Add 24 hours' worth of seconds
    }

    return time_difference;
}

bool gpsData::CheckDataAge() {
    int age = CalculateTimeDifferenceInSeconds();
    SetDataAge(age);

    if (age > MaxDataAgeSeconds) {
        SetDataReliability(false);
        return false;
    } else {
        return true;
    }
}

void gpsData::SetDataAge(int newDataAge) {
    std::lock_guard<std::mutex> lock(mtx);
    if (newDataAge != main_data_age_seconds) {
        main_data_age_seconds = newDataAge;
        NotifyDataAge();
    }
}

int gpsData::GetDataAge() const {
    std::lock_guard<std::mutex> lock(mtx);
    return main_data_age_seconds;
}

void gpsData::NotifyDataAge() {
    for (Observer* observer : observers) {
        observer->UpdateDataAge(main_data_age_seconds);
    }
}

std::string gpsData::GetFullStatusReport() {
    std::ostringstream oss;
    int age = CalculateTimeDifferenceInSeconds();

    oss << "=== GPS Data Full Report ===\n";
    oss << "Latitude: " << main_data.lat << "\n";
    oss << "Longitude: " << main_data.lon << "\n";
    oss << "HDOP: " << main_data.hdop << "\n";
    oss << "UTC: " << main_data.utc << "\n";
    oss << "Altitude: " << main_data.alt << "\n";
    oss << "SOG: " << main_data.sog << "\n";
    oss << "COG: " << main_data.cog << "\n";
    oss << "Fix: " << static_cast<int>(main_data.fix) << "\n";
    oss << "Satellite Count: " << static_cast<int>(main_data.satellite_count) << "\n";
    oss << "RMC Valid: " << (main_data.rmc_valid ? "true" : "false") << "\n";
    oss << "Data Reliability: " << (main_data.data_reliability ? "true" : "false") << "\n";
    oss << "Data Age (s): " << age << "\n";
    oss << "MaxDataAgeSeconds: " << MaxDataAgeSeconds << "\n";
    oss << "==============================";

    return oss.str();
}

void gpsData::CalculateAndUpdateDistance() {
    constexpr double NOISE_THRESHOLD_NM = 0.00108; // Entspricht ca. 2 m

    if (GetDataReliability()) {
        if (old_data_wasreliable)
        {
            double nm = CalculateDistanceNm(old_lat, old_lon, main_data.lat, main_data.lon);
            //logToDebugger("Calculated distance: ", nm);

            if (nm > NOISE_THRESHOLD_NM) {
                old_lat = main_data.lat;
                old_lon = main_data.lon;

                SetTripDist(GetTripDist() + nm);
            }
        }
        else {
            old_data_wasreliable = true;
            old_lat = main_data.lat;
            old_lon = main_data.lon;
        }
    }
}

void gpsData::ResetTripDist() {
    old_data_wasreliable = false;
    old_lat = 0;
    old_lon = 0;
    SetTripDist(0.0);
}