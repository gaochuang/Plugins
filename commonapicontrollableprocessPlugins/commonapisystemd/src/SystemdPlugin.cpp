#include "SystemdPlugin.hpp"

using namespace systemdPlugin;

SystemdPlugin::SystemdPlugin(std::shared_ptr<commonApi::PluginServices> pluginServices):
                pluginServices(pluginServices),
                fdMonitor(pluginServices->getFdMonitor()),
                signalMonitor(pluginServices->getSignalMonitor()),
                callbackQueue(pluginServices->getEventCallbackQueue()),
                timerService(pluginServices->getTimerService())

{

}
