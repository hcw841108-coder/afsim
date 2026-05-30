#include "wsf_onboard_computer_export.h"
#include "WsfPlugin.hpp"
#include "WsfApplication.hpp"
#include "WsfApplicationExtension.hpp"
#include "OnboardComputerExtension.hpp"

extern "C"
{
    WSF_ONBOARD_COMPUTER_EXPORT void WsfPluginVersion(UtPluginVersion& aVersion)
   {
      aVersion = UtPluginVersion(WSF_PLUGIN_API_MAJOR_VERSION,
                                 WSF_PLUGIN_API_MINOR_VERSION,
                                 WSF_PLUGIN_API_COMPILER_STRING);
   }

    WSF_ONBOARD_COMPUTER_EXPORT void WsfPluginSetup(WsfApplication& aApplication)
   {
      if (!aApplication.ExtensionIsRegistered("wsf_onboard_computer"))
      {
          aApplication.RegisterFeature("onboard_computer", "wsf_onboard_computer");
          //WSF_REGISTER_EXTENSION(aApplication, wsf_mil); // This extension REQUIRES the "wsf_mil" extension
          aApplication.RegisterExtension("wsf_onboard_computer", ut::make_unique<WsfDefaultApplicationExtension<OnboardComputerExtension>>());
      }
   }
}
