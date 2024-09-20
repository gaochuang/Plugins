#include "comApi/comApi.hpp"
#include "comapi/controllableProcess/controllableProcessPlugin.hpp"
#include "comapi/controllableProcess/controllableProcess.hpp"

#include <iostream>

using namespace reactorFramework;

int main()
{
    auto api = ComAPI::create();
    std::cout << "this is test " <<std::endl;

    auto controllableProcess = controllableprocess::ControllableProcess::create(api);

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