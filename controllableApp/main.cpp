#include "ComAPI.hpp"
#include "controllableProcess/ControllableProcessPlugin.hpp"
#include "controllableProcess/ControllableProcess.hpp"

#include <iostream>

using namespace commonApi;

int main()
{
    auto api = ComAPI::create();
    std::cout << "this is test " <<std::endl;

    //使用commonapi daemon 的Plugin
    setenv("DAEMON_PROCESS_TYPE", "daemon", 1);

    auto controllableProcess = controllableprocess::ControllableProcess::create(api);
    controllableProcess->notifyReady();
    controllableProcess->setTerminateCb(
        [&controllableProcess]()
        {
            controllableProcess->heartbeatAck();
        }
    );

    controllableProcess->setTerminateCb(
        []()
        {
            std::cout << "clean resource" << std::endl;
        }
    );

    while(true)
    {
        api->waitAndHandleEvents();
    }
    return 0;
}
