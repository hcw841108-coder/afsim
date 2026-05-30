#ifndef ONBOARD_COMPUTER_EXTENSION_HPP
#define ONBOARD_COMPUTER_EXTENSION_HPP

#include "WsfScenarioExtension.hpp"
#include "WsfSimulation.hpp"
#include "UtMemory.hpp"
#include "OnboardComputer.hpp"

class OnboardComputerExtension : public WsfScenarioExtension
{
public:
	~OnboardComputerExtension() noexcept override = default;

private:
	//void AddedToScenario();
	bool ProcessInput(UtInput& aInput);
	void FileLoaded(const std::string& aFileName);
	//bool Complete();
	//bool Complete2();
	void SimulationCreated(WsfSimulation& aSimulation) override;
	//bool AlwaysCreate();

private:
	XDU::OnboardComputer mOnboardComputer;
};

#endif
