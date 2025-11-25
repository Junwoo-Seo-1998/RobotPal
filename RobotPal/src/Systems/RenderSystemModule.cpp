#include "RobotPal/Systems/RenderSystemModule.h"

RenderSystemModule::RenderSystemModule(flecs::world &world)
{
    world.module<RenderSystemModule>();
}

void RenderSystemModule::RegisterSystem(flecs::world &world)
{
    //todo
}