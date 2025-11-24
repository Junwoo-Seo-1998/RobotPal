#ifndef __TRANSFORMSYSTEMGROUP_H__
#define __TRANSFORMSYSTEMGROUP_H__
#include "RobotPal/Systems/SystemGroup.h"
class TransformSystemGroup : public ISystemGroup {
public:
    TransformSystemGroup();
    void Register(flecs::world& world) override;
};

#endif