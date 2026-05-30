#include "OnboardComputerExtension.hpp"
#include "PlatformDataCollector.hpp"

bool OnboardComputerExtension::ProcessInput(UtInput& aInput)
{
    return mOnboardComputer.ProcessInput(aInput);
}

void OnboardComputerExtension::FileLoaded(const std::string& aFileName)
{
    int a = 0;
}

void OnboardComputerExtension::SimulationCreated(WsfSimulation& aSimulation)
{
    aSimulation.RegisterExtension("wsf_onboard_computer",
        ut::make_unique<XDU::OnboardComputer>(mOnboardComputer));

    aSimulation.RegisterExtension("platform_data_collector",
        ut::make_unique<PlatformDataCollector>());
}