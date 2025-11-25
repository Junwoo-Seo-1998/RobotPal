#ifndef __TRANSFORMSYSTEMGROUP_H__
#define __TRANSFORMSYSTEMGROUP_H__
#include <flecs.h>
struct TransformSystemModule{
public:
    TransformSystemModule(flecs::world &world);
private:
    void RegisterObserver(flecs::world& world);
    void RegisterSystem(flecs::world& world);
};

#endif