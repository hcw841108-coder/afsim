#ifndef ONBOARD_COMPUTER_HPP
#define ONBOARD_COMPUTER_HPP

#include "wsf_onboard_computer_export.h"
#include "WsfScenarioExtension.hpp"
#include "UtCallbackHolder.hpp"
#include "OnboardComputerEvents.hpp"
#include "WsfComm.hpp"
#include "TcpServer.hpp"
#include "UdpUnicastReceiver.hpp"
#include <QUdpSocket>

namespace XDU
{
	//using AcceptedCallBack = std::function<GenTCP_IO* ()>;

	class WSF_ONBOARD_COMPUTER_EXPORT OnboardComputer : public WsfSimulationExtension/*, public QObject*/
	{
		//Q_OBJECT

	public:
		OnboardComputer(/*QObject* parent = nullptr*/);
		OnboardComputer(const OnboardComputer& aSrc);
		OnboardComputer& operator=(const OnboardComputer& aSrc);
		~OnboardComputer() noexcept override;

		bool ProcessInput(UtInput& aInput);
		void AddedToSimulation() override;
		bool Initialize() override;
		bool PrepareExtension() override;
		//bool PlatformsInitialized() override;
		void PendingStart() override;
		void Start() override;
		void Complete(double aSimTime) override;

	private:
		void Update(double aSimTime);

		void OnPlatformInitialized(double aSimTime, WsfPlatform* aPlatformPtr);
		//void OnMessageReceived(double aSimTime, wsf::comm::Comm* aXmtrPtr, wsf::comm::Comm* aRcvrPtr, const WsfMessage& aMessage, wsf::comm::Result& aResult);

		void OnMessageReceived(const char* aData, std::size_t aSize, const udp::endpoint& aEndPoint);
		void ProcessMessage(std::string aMessageStr);

		void OnUdpSocketReadyRead();

	private:
		UtCallbackHolder mCallbacks;
		int                  mPort;

		boost::asio::io_context io_context;
		UdpUnicastReceiver* mUdpUnicastReceiverPtr = nullptr;


		std::thread mIOThread;
		std::thread mAcceptThread;
		std::thread mReceiveThread;

		std::atomic<bool> mIsAccept;
		std::atomic<bool> mIsReceive;

		std::mutex mMutex;

		QUdpSocket* mUdpSocketPtr = nullptr;
	};
}
#endif // ONBOARD_COMPUTER_HPP
