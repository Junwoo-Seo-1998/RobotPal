#ifndef __RENDERSYSTEMGROUP_H__
#define __RENDERSYSTEMGROUP_H__
#include "RobotPal/Systems/SystemGroup.h"
class RenderSystemGroup : public ISystemGroup {
public:
    RenderSystemGroup();

    void Register(flecs::world& world) override;
};

#endif