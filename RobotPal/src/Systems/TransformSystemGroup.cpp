#include "Robotpal/Systems/TransformSystemGroup.h"
#include "RobotPal/Components/Components.h"
#include <flecs.h>

TransformSystemGroup::TransformSystemGroup()
{
}

void TransformSystemGroup::Register(flecs::world &world)
{
    // 1. 템플릿 없이 기본 빌더 사용
    auto sys = world.system<TransformComponent>("UpdateTransformSystem");

    sys.each([](TransformComponent& transform){
        transform.WorldMatrix=transform.GetLocalMatrix();
    });
}