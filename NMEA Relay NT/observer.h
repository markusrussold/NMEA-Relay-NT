#pragma once

#ifndef OBSERVER_H
#define OBSERVER_H

class Observer {
public:
    virtual void UpdateLatLon(double lat, double lon) = 0;
    virtual void UpdateHdop(double hdop) = 0;
    virtual void UpdateUtc(double utc) = 0;
    virtual void UpdateAlt(double alt) = 0;
    virtual void UpdateFix(uint8_t fix) = 0;
    virtual void UpdateSatelliteCount(uint8_t satellite_count) = 0;
    virtual void UpdateRmcValidity(bool rmc_valid) = 0;
    virtual void UpdateSog(float sog) = 0;
    virtual void UpdateCog(float cog) = 0;
    virtual void UpdateDataReliability(bool data_reliability) = 0;
    virtual void UpdateDataAge(int newDataAge) = 0;
    virtual void UpdateTripDist(double tripdist) = 0;

};

#endif // OBSERVER_H