#include "GetPlatformData.hpp"


GetPlatformData::GetPlatformData()
	: mPlatformcount(0)
{
}

GetPlatformData::~GetPlatformData()
{
	Shutdown();
}

//===================== 收集单个平台数据 =====================
bool GetPlatformData::SingleCollectPlatformData(WsfPlatform* platform, PlatformData& data)
{
	if (platform == nullptr)
	{
		return false;
	}
	data.platformID = static_cast<int>(platform->GetIndex());  //索引
	data.name = platform->GetName();      //名称
	data.side = platform->GetSide();      //阵营
	data.typeId = platform->GetTypeId();  
	data.platformType = data.typeId.GetString();  //类型
	platform->GetLocationLLA(data.lat, data.lon, data.alt);  //位置

	double heading_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rad = 0.0;
	platform->GetOrientationNED(heading_rad, pitch_rad, roll_rad);  
	data.heading = heading_rad * UtMath::cDEG_PER_RAD;
	data.pitch = pitch_rad * UtMath::cDEG_PER_RAD;
	data.roll = roll_rad * UtMath::cDEG_PER_RAD;
	data.yaw = data.heading;                      //姿态角（弧度转角度）


	data.speed_mps = platform->GetSpeed();  //速度
	// 获取机体坐标系加速度
	auto accelEcs = platform->GetAccelerationECS();
	ut::Vec3<double, ut::detail::Vec3Tag> vecAccel = accelEcs;

	data.accelX_ecs = vecAccel[0];
	data.accelY_ecs = vecAccel[1];
	data.accelZ_ecs = vecAccel[2];

	// 加速度   m/s^2 转换为 g
	data.accelX_g = data.accelX_ecs / UtEarth::cACCEL_OF_GRAVITY;
	data.accelY_g = data.accelY_ecs / UtEarth::cACCEL_OF_GRAVITY;
	data.accelZ_g = data.accelZ_ecs / UtEarth::cACCEL_OF_GRAVITY;

	data.isBroken = platform->IsBroken();  //是否损毁

	return true;
}

//===================== 收集所有平台数据 =====================
bool GetPlatformData::CollectAllPlatformData(WsfSimulation& sim)
{
	//清空数据
	mAllPlatforms.clear();

	//获取平台数量
	mPlatformcount = sim.GetPlatformCount();

	mAllPlatforms.reserve(mPlatformcount); // 预分配空间，提升性能

	//遍历平台并收集数据
	for (size_t i = 0; i < mPlatformcount; ++i)
	{
		PlatformData data{};
		WsfPlatform* platform = sim.GetPlatformEntry(i);

		if (!platform)
		{
			continue;
		}

		if (SingleCollectPlatformData(platform, data))
		{
			mAllPlatforms.push_back(data);
		}
	}
	double simTime = GetSimulation().GetSimTime();
	if(!mAllPlatforms.empty())
	{
		std::cout << "[GetPlatformData] 数据已采集" << std::endl;
		std::cout << "[GetPlatformData] 仿真时间 = "
			<< simTime;
		std::cout << "[GetPlatformData] 平台数量 = "
			<< mAllPlatforms.size()
			<< std::endl;
	}
	else
	{
		std::cout << "未收集到平台数据，[GetPlatformData]为空"
			<< std::endl;
	}

	return true;
}

//===================== 清理 =====================
void GetPlatformData::Shutdown()
{
	if (mShutDown) {
		return;
	}
	mShutDown = true;

	mAllPlatforms.clear();
}

//===================== 仿真开始 =====================
void GetPlatformData::Start()
{
	mShutDown = false;
	std::cout<<"仿真开始"<<std::endl;

	WsfSimulation& sim = GetSimulation();
	CollectAllPlatformData(sim);
}

//===================== 仿真结束 =====================
void GetPlatformData::Complete(double aSimTime)
{
	std::cout<<"仿真时间 = " << aSimTime << std::endl;
	std::cout << "仿真结束" << std::endl;
}