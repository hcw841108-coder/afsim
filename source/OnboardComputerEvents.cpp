#include "OnboardComputerEvents.hpp"
//#include "WsfSpaceMover.hpp"
#include "WsfZoneTypes.hpp"
#include "WsfZoneDefinition.hpp"
#include "WsfFieldOfView.hpp"
#include "WsfCommNetworkManager.hpp"

#include "UtSleep.hpp"

WsfEvent::EventDisposition XDU::UpdateEvent::Execute()
{
	WsfEvent::EventDisposition eventDisposition = WsfEvent::EventDisposition::cDELETE;

	WsfSimulation::State state = GetSimulation()->GetState();
	if (state != WsfSimulation::State::cPENDING_COMPLETE)
	{
		double rescheduleTime = GetTime();
		mUpdateCallBack(rescheduleTime);

		rescheduleTime += mUpdateInterval;
		SetTime(rescheduleTime);
		eventDisposition = WsfEvent::cRESCHEDULE;
	}
	return eventDisposition;
}