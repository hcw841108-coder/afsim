#ifndef GETPLATFORMDATA_HPP
#define GETPLATFORMDATA_HPP

#include "WsfPlatform.hpp"
#include "WsfSimulation.hpp"
#include "WsfSimulationExtension.hpp"
#include "string"
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>

struct PlatformData
{
	int platformID;
	std::string name;
	std::string side;
	WsfStringId typeId;
	std::string platformType;
	double lat;  //纬度
	double lon;  //经度
	double alt;  //高度

	double heading;   // 角度
	double yaw;       // deg
	double pitch;     // deg
	double roll;      // deg
	
	double speed_mps; // 速度

	double accelX_ecs; //加速度_X，m/s^2
	double accelY_ecs; //加速度_Y，m/s^2
    double accelZ_ecs; //加速度_Z，m/s^2

	double accelX_g;   //加速度_X，g
	double accelY_g;   //加速度_Y，g
	double accelZ_g;   //加速度_Z，g

	bool isBroken;     //是否损毁
};

class GetPlatformData : public WsfSimulationExtension
{
public:
	GetPlatformData();
	~GetPlatformData();



private:
	bool SingleCollectPlatformData(WsfPlatform* platform, PlatformData& data);
	bool CollectAllPlatformData(WsfSimulation& sim, std::vector<PlatformData>& outData);
	int mPlatformcount;
	std::vector<PlatformData> mAllPlatforms;

};














#endif