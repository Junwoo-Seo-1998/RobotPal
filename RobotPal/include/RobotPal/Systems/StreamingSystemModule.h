#ifndef __STREAMINGSYSTEMMODULE_H__
#define __STREAMINGSYSTEMMODULE_H__
#include "RobotPal/Network/NetworkEngine.h"
#include <flecs.h>

struct StreamingSystemModule
{
public:
    StreamingSystemModule(flecs::world &world);
private:
    void RegisterObserver(flecs::world& world);
    void RegisterSystem(flecs::world& world);


    NetworkEngine* netEngine;
};

#endif