#ifndef __STREAMINGSYSTEMMODULE_H__
#define __STREAMINGSYSTEMMODULE_H__

#include <flecs.h>

class NetworkEngine;

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