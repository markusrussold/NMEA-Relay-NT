#pragma once

#ifndef GPSDATA_H
#define GPSDATA_H

#include <mutex>  // Include mutex header
#include <vector>
#include "Observer.h"
#include <iostream>
#include <utility>
#include <stdexcept>
#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cmath>
#include "Constants.h"
#include "Globals.h"

class gpsData
{
    public:
        std::vector<Observer*> observers;
        mutable std::mutex mtx;  // Mutex to protect shared data
        int MaxDataAgeSeconds = 0;
        int main_data_age_seconds = 0;

        struct pos_data_struct {
            double utc;
            double lat;
            double lon;
        };

        struct main_data_struct {
            double lat;
            double lon;
            double hdop;
            double utc;
            double alt;
            float sog;
            float cog;
            uint8_t fix;
            uint8_t satellite_count;
            bool rmc_valid;
            bool data_reliability;
        };

        main_data_struct main_data;
        std::vector<pos_data_struct> pos_data_list;

public:
    // Constructor
    gpsData() {
        MaxDataAgeSeconds = MAX_DATA_AGE_SECONDS;
        main_data.lat = 0.0;
        main_data.lon = 0.0;
        main_data.hdop = 100.0;
        main_data.utc = 0.0;
        main_data.alt = -1.0;
        main_data.fix = -1;
        main_data.satellite_count = 0;
        main_data.cog = -1.0;
        main_data.sog = -1.0;
        main_data.rmc_valid = false;
        main_data.data_reliability = false;
    }

    std::pair<double, double> ProvideSmoothenedPosition(const std::vector<pos_data_struct>& pos_data_list) const;
    void Attach(Observer* observer);
    void Detach(Observer* observer);
    void NotifyLatLon();
    void NotifyHdop();
    void NotifyUtc();
    void NotifyAlt();
    void NotifyFix();
    void NotifySatelliteCount();
    void NotifyRmcValidity();
    void NotifySog();
    void NotifyCog();
    void NotifyDataReliability();

    // GPS data setters
    void SetMaxDataAgeSeconds(int MaxDataAgeSecondsInput);
    void SetData(double newLat, double newLon, double newHdop, double newUtc, double newAlt, uint8_t newFix);
    //void SetLatitude(double newLat);
    //void SetLongtitude(double newLon);
    void SetHdop(double newHdop);
    void SetUtc(double newUtc);
    void SetAlt(double newAlt);
    void SetFix(uint8_t newFix);
    void SetSatelliteCount(uint8_t newSatelliteCount);
    //void SetRmcValidity(bool newRmcValidity);
    void SetSog(float newSog);
    void SetCog(float newCog);
    void SetDataReliability(bool newData_reliability);

    // GPS data getters
    main_data_struct GetData();
    double GetLatitude() const;
    double GetLongitude() const;
    double GetHdop() const;
    double GetUtc() const;
    double GetAlt() const;
    uint8_t GetFix() const;
    uint8_t GetSatelliteCount() const;
    bool GetRmcValidity() const;
    float GetSog() const;
    float GetCog() const;
    bool GetDataReliability() const;
    std::string GetAllMainData();
    int GetMaxDataAgeSeconds() const;

    void ReportSentenceComplete();
    bool CheckDataAge();
    void SetDataAge(int newDataAge);
    int GetDataAge() const;
    void NotifyDataAge();
    std::string GetFullStatusReport();
    int CalculateTimeDifferenceInSeconds();
    double GetCurrentUtcTimeAsDouble();
};

#endif // GPSDATA_H