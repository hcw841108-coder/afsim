#include "OnboardComputer.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

// Utilities
#include "UtCast.hpp"
#include "UtInputBlock.hpp"
#include "UtLog.hpp"
#include "UtSolarSystem.hpp"
#include "WsfCommNetworkManager.hpp"
//#include "WsfSpaceMover.hpp"

#include "GenTCP_IO.hpp"
#include "GenBufIManaged.hpp"

#include "rapidjson/rapidjson.h"

// ============================================================================
//! 构造OnboardComputer类
XDU::OnboardComputer::OnboardComputer()
	: mPort(8885)
	, mUdpUnicastReceiverPtr(nullptr)
	//, mUdpSocketPtr(new QUdpSocket)
{
}

//! 拷贝构造OnboardComputer类
XDU::OnboardComputer::OnboardComputer(const OnboardComputer& aSrc)
	: mPort(aSrc.mPort)
	, mUdpUnicastReceiverPtr(nullptr)
	//, mUdpSocketPtr(new QUdpSocket)
{

}

XDU::OnboardComputer& XDU::OnboardComputer::operator=(const OnboardComputer& aSrc)
{
	mPort = aSrc.mPort;
	//mUdpSocketPtr = aSrc.mUdpSocketPtr;
	return *this;
}

//! 析构OnboardComputer类
XDU::OnboardComputer::~OnboardComputer() noexcept
{
	mCallbacks.Clear();
}

bool XDU::OnboardComputer::ProcessInput(UtInput& aInput)
{
	return true;
}

void XDU::OnboardComputer::AddedToSimulation()
{

}

bool XDU::OnboardComputer::Initialize()
{
	mCallbacks.Clear();

	//if (mUdpSocketPtr->bind(mPort))
	//{
	//	QObject* mContextPtr = new QObject;
	//	bool res = QObject::connect(mUdpSocketPtr, &QUdpSocket::readyRead, mContextPtr, [this]() /*&XDU::OnboardComputer::OnUdpSocketReadyRead);*/
	//	{
	//		while (mUdpSocketPtr->hasPendingDatagrams())
	//		{
	//			QByteArray datagram;
	//			datagram.resize(mUdpSocketPtr->pendingDatagramSize());

	//			QHostAddress peerAddr;
	//			quint16 peerPort;
	//			mUdpSocketPtr->readDatagram(datagram.data(), datagram.size(), &peerAddr, &peerPort);
	//			QString str = datagram.data();

	//			QString peer = "[消息来自 " + peerAddr.toString() + ":" + QString::number(peerPort) + "] | ";
	//		}
	//	});
	//}

	//TcpServer s(io_context, mPort);
	mUdpUnicastReceiverPtr = new UdpUnicastReceiver(io_context, mPort);
	mUdpUnicastReceiverPtr->SetReceivedCallback(std::bind(&XDU::OnboardComputer::OnMessageReceived, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	mIOThread = std::thread([&]()
	{
		std::cout << "IO 线程启动（ID：" << std::this_thread::get_id() << "）" << std::endl;
		io_context.run(); // 阻塞在 IO 线程中
		std::cout << "IO 线程退出" << std::endl;
	});
	mIOThread.detach();

	mCallbacks += WsfObserver::PlatformInitialized(&GetSimulation()).Connect(&XDU::OnboardComputer::OnPlatformInitialized, this);
	//mCallbacks += WsfObserver::MessageReceived(&GetSimulation()).Connect(&XDU::OnboardComputer::OnMessageReceived, this);

	//mUpdateCallBack = [this](double aSimTime) { Update(aSimTime); };
	//mUpdateCallBack = std::bind(&XDU::OnboardComputer::Update, this, std::placeholders::_1);

	return true;
}

bool XDU::OnboardComputer::PrepareExtension()
{
	return true;
}

void XDU::OnboardComputer::PendingStart()
{
	GetSimulation().AddEvent(ut::make_unique<WsfRecurringEvent>(GetSimulation().GetSimTime(), [=](WsfEvent& e)
	{
		e.SetTime(e.GetTime() + 0.01);
		return WsfEvent::cRESCHEDULE;
	}));
}

void XDU::OnboardComputer::Start()
{
	// 
	//size_t platformCount = GetSimulation().GetPlatformCount();
	//for (size_t i = 0; i < platformCount; i++)
	//{
	//	WsfPlatform* platformPtr = GetSimulation().GetPlatformEntry(i);
	//	if (platformPtr->IsInitialized())
	//	{
	//		std::string platformName = platformPtr->GetName();
	//		double index = platformPtr->GetIndex();
	//		if (platformPtr->IsA_TypeOf("SATELLITE"))
	//		{
	//			std::ofstream* outFilePtr = new std::ofstream(platformName + ".csv");
	//			if (outFilePtr->is_open()) {
	//				// 写入CSV表头
	//				*outFilePtr << "Time,Lon,lat,Alt" << std::endl;
	//				satMap[platformPtr] = outFilePtr;
	//			}
	//		}
	//	}
	//}
}

void XDU::OnboardComputer::Complete(double aSimTime)
{
	mCallbacks.Clear();
}

void XDU::OnboardComputer::Update(double aSimTime)
{

}

void XDU::OnboardComputer::OnPlatformInitialized(double aSimTime, WsfPlatform* aPlatformPtr)
{

}


void XDU::OnboardComputer::OnMessageReceived(const char* aData, std::size_t aSize, const udp::endpoint& aEndPoint)
{
	std::string str(aData, aSize);
	delete[] aData;
	std::cout << "Received:" << str << std::endl;
	ProcessMessage(str);
}

void XDU::OnboardComputer::ProcessMessage(std::string aMessageStr)
{
	//type = attitude, ts = 2025 - 12 - 20T16:47 : 04.819, roll = 95.98, pitch = 27.99, yaw = 92.16
	std::string token;
	std::vector<std::string> valueVec;
	std::istringstream iss(aMessageStr);
	while (std::getline(iss, token, ','))
	{
		token.erase(std::remove(token.begin(), token.end(), ' '), token.end());
		valueVec.push_back(token);
	}

	std::string typeValue = valueVec.at(0);
	std::istringstream iss_type(typeValue);
	std::vector<std::string> typePair;
	while (std::getline(iss_type, token, '='))
	{
		typePair.push_back(token);
	}
	if (typePair.size() < 2)
	{
		return;
	}
	if (typePair[1] == "attitude")
	{
		double roll = 0.0, pitch = 0.0, yaw = 0.0;
		int threadId = 0; // 【核心新增】定义threadID，默认0（解析失败时使用sat-SAR0）
		for (size_t i = 1; i < valueVec.size(); i++)
		{
			std::istringstream iss(valueVec.at(i));
			std::vector<std::string> pair;
			while (std::getline(iss, token, '='))
			{
				pair.push_back(token);
			}
			if (pair.at(0) == "roll")
			{
				roll = std::stod(pair.at(1));
			}
			else if (pair.at(0) == "pitch")
			{
				pitch = std::stod(pair.at(1));
			}
			else if (pair.at(0) == "yaw")
			{
				yaw = std::stod(pair.at(1));
			}
			// 【核心新增】解析thread字段为整数ID，增加异常捕获防止格式错误
			else if (pair[0] == "thread")
			{
				try
				{
					threadId = std::stoi(pair[1]); // 转整数，匹配thread的数字值
				}
				catch (const std::invalid_argument&)
				{
					threadId = 0; // 解析失败默认归为0
				}
				catch (const std::out_of_range&)
				{
					threadId = 0;
				}
			}
		}

		// 【核心修改】动态拼接平台名：sat-SAR + threadId → sat-SAR0、sat-SAR1...
		std::string platformName = "plane" + std::to_string(threadId);
		WsfPlatform* satPlatformPtr = GetSimulation().GetPlatformByName(platformName);

		if (!satPlatformPtr)
		{
			return;
		}
		satPlatformPtr->SetOrientationNED(yaw, pitch, roll);
		satPlatformPtr->Update(GetSimulation().GetSimTime());
	}
	else if (typePair[1] == "xiaoxuan")
	{
		double roll = 0.0, pitch = 0.0, yaw = 0.0;
		int threadId = 0; // 【核心新增】定义threadID，默认0（解析失败时使用sat-SAR0）
		for (size_t i = 1; i < valueVec.size(); i++)
		{
			std::istringstream iss(valueVec.at(i));
			std::vector<std::string> pair;
			while (std::getline(iss, token, '='))
			{
				pair.push_back(token);
			}
			if (pair.at(0) == "roll")
			{
				roll = std::stod(pair.at(1)) * UtMath::cPI / 180;
			}
			else if (pair.at(0) == "pitch")
			{
				pitch = std::stod(pair.at(1));
			}
			else if (pair.at(0) == "yaw")
			{
				yaw = std::stod(pair.at(1));
			}
			// 【核心新增】解析thread字段为整数ID，增加异常捕获防止格式错误
			else if (pair[0] == "thread")
			{
				try
				{
					threadId = std::stoi(pair[1]); // 转整数，匹配thread的数字值
				}
				catch (const std::invalid_argument&)
				{
					threadId = 0; // 解析失败默认归为0
				}
				catch (const std::out_of_range&)
				{
					threadId = 0;
				}
			}
		}

		// 【核心修改】动态拼接平台名：sat-SAR + threadId → sat-SAR0、sat-SAR1...
		std::string platformName = "sat-SAR" + std::to_string(threadId);
		WsfPlatform* satPlatformPtr = GetSimulation().GetPlatformByName(platformName);

		if (!satPlatformPtr)
		{
			return;
		}
		satPlatformPtr->SetOrientationNED(yaw, pitch, roll);
		satPlatformPtr->Update(GetSimulation().GetSimTime());

		if (threadId == 9) {
			GetSimulation().SetRealtime(GetCurrentTime() + 1, true);
		}
	}
	else if (typePair[1] == "camera")
	{
		WsfPlatform* satPlatformPtr = GetSimulation().GetPlatformByName("sat-SAR");
		if (satPlatformPtr)
		{
			WsfPlatformComponent* componentPtr = satPlatformPtr->FindComponent("sar_sensor", cWSF_COMPONENT_SENSOR);
			if (componentPtr)
			{
				WsfSensor* sensorPtr = dynamic_cast<WsfSensor*>(componentPtr);
				if (sensorPtr)
				{
					if (!sensorPtr->IsTurnedOn())
					{
						GetSimulation().TurnPartOn(GetSimulation().GetSimTime(), sensorPtr);
					}
					else
					{
						GetSimulation().TurnPartOff(GetSimulation().GetSimTime(), sensorPtr);
					}
				}
			}
		}
	}
}

void XDU::OnboardComputer::OnUdpSocketReadyRead()
{

}
