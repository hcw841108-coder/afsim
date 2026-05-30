#ifndef PLATFORM_DATA_COLLECTOR_HPP
#define PLATFORM_DATA_COLLECTOR_HPP

#include "WsfSimulationExtension.hpp"
#include "WsfSimulation.hpp"
#include "WsfPlatform.hpp"
#include "WsfMoverObserver.hpp"
#include "UtCallbackN.hpp"
#include "WsfSensor.hpp"
#include "WsfSensorMode.hpp"


#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>

#include "UdpJsonSender.hpp"

struct SensorData
{
    std::string name;
    std::string type;

    size_t currentModeIndex;
    size_t modeCount;

    double maximumRange_m;

    bool hasTargetInRange;
    int targetCountInRange;
};

struct PlatformData
{
    int id;
    std::string name;
    std::string side;
    std::string platformType;
    double lat;
    double lon;
    double alt;

    double heading;   // deg
    double yaw;       // deg
    double pitch;     // deg
    double roll;      // deg

    double speed_mps;

    double accelX_ecs; // m/s^2
    double accelY_ecs; // m/s^2
    double accelZ_ecs; // m/s^2

    double accelX_g;
    double accelY_g;
    double accelZ_g;

    //double mass_kg;

    //double fuelTotalCapacity_kg;
    //double fuelInternalCapacity_kg;
    //double fuelExternalCapacity_kg;

    //double fuelTotalRemaining_kg;
    //double fuelInternalRemaining_kg;
    //double fuelExternalRemaining_kg;

    //double fuelFlow_kgps;
    //double fuelPercent;

    bool isBroken;

    std::vector<SensorData> sensors;
};

class PlatformDataCollector : public WsfSimulationExtension
{
public:
    PlatformDataCollector();
    ~PlatformDataCollector() override;

    void AddedToSimulation() override;
    bool Initialize() override;
    bool PlatformsInitialized() override;
    void Start() override;
    void Complete(double aSimTime) override;

    void Shutdown();

    const std::vector<PlatformData>& GetAllPlatformData() const;

private:
    bool ExtractSinglePlatformData(WsfPlatform* platform, PlatformData& outData);
    void ExtractSensorData(WsfPlatform* platform, std::vector<SensorData>& outSensors);
    double GetSensorMaximumRange(WsfSensor* sensor);

    void PrintCollectedData(const std::vector<PlatformData>& data, double time);
    void CollectAndSend(double simTime);
    void OnMoverUpdated(double simTime, WsfMover* moverPtr);

private:
    std::vector<PlatformData> mAllPlatformData;
    UdpJsonSender mUdpSender;
    void HandleAgentCommand(const std::string& jsonText, double simTime);
    double mSendInterval;
    double mNextSendTime;
    bool mHasSentStartFrame;
    bool mIsShuttingDown;

    std::unique_ptr<WsfObserver::MoverUpdatedCallback::CallbackType> mMoverUpdatedCallback;
};

#endif