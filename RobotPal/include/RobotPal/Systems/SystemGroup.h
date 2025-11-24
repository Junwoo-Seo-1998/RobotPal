#ifndef __SYSTEMGROUP_H__
#define __SYSTEMGROUP_H__
#include <flecs.h>
class ISystemGroup 
{
public:
    virtual ~ISystemGroup() = default;
    virtual void Register(flecs::world& world) = 0;
};

#endif