#ifndef __RENDERSYSTEMMODULE_H__
#define __RENDERSYSTEMMODULE_H__
#include <flecs.h>
struct RenderSystemModule{
public:
    RenderSystemModule(flecs::world &world);

private:
    void RegisterSystem(flecs::world& world);
};

#endif