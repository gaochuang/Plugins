#include "ComAPI.hpp"
#include "controllableProcess/ControllableProcessPlugin.hpp"
#include "controllableProcess/ControllableProcess.hpp"

#include <iostream>

using namespace commonApi;

int main()
{
    auto api = ComAPI::create();
    std::cout << "this is test " <<std::endl;

    bool runFlag(true);

    //使用commonapi daemon 的Plugin
    setenv("DAEMON_PROCESS_TYPE", "daemon", 1);

    auto controllableProcess = controllableprocess::ControllableProcess::create(api);
    controllableProcess->notifyReady();
    controllableProcess->setHeartbeatCb(
        [&controllableProcess]()
        {
            controllableProcess->heartbeatAck();
        }
    );

    controllableProcess->setTerminateCb(
        [&runFlag]()
        {
            std::cout << "clean resource" << std::endl;
            runFlag = false;
        }
    );

    while(runFlag)
    {
        api->waitAndHandleEvents();
    }
    return 0;
}
