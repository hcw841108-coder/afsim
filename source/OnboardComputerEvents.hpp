#ifndef ONBOARD_COMPUTER_EVENTS_HPP
#define ONBOARD_COMPUTER_EVENTS_HPP

#include "WsfEvent.hpp"

namespace XDU
{
	using UpdateCallBack = std::function<void(double)>;

	class UpdateEvent : public WsfEvent
	{
	public:
		UpdateEvent(const UpdateCallBack& aUpdateCallBack, double aUpdateInterval = 0.05)
			: mUpdateCallBack(aUpdateCallBack)
			, mUpdateInterval(aUpdateInterval)
		{
		}

		EventDisposition Execute() override;

	private:
		const UpdateCallBack& mUpdateCallBack;
		double mUpdateInterval;
	};
}
#endif // ONBOARD_COMPUTER_EVENTS_HPP
