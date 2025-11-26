#include "Robotpal/Systems/TransformSystemModule.h"
#include "RobotPal/Components/Components.h"
#include <flecs.h>

TransformSystemModule::TransformSystemModule(flecs::world &world)
{
    world.module<TransformSystemModule>();

    world.component<Position>("Position");
    world.component<Rotation>("Rotation");
    world.component<Scale>("Scale");
    world.component<TransformMatrix>("TransformMatrix");
    world.component<Local>("Local");
    world.component<World>("World");

    RegisterObserver(world);
    RegisterSystem(world);
}

void TransformSystemModule::RegisterObserver(flecs::world &world)
{
    // world set 설정하기
}

void TransformSystemModule::RegisterSystem(flecs::world &world)
{
    // 1. 템플릿 없이 기본 빌더 사용
    auto update_local = world.system<const Position, const Rotation, const Scale, TransformMatrix>()
    .kind(flecs::OnUpdate)
    .term_at(0).second<Local>() // (Position, Local)
    .term_at(1).second<Local>() // (Rotation, Local)
    .term_at(2).second<Local>() // (Scale, Local)
    .term_at(3).second<Local>() // (TransformMatrix, Local) -> 여기에 씀
    .each([](flecs::entity entity, const Position& p, const Rotation& r, const Scale& s, TransformMatrix& m) {
        // Local Matrix = Translate * Rotate * Scale
        m = glm::translate(glm::mat4(1.0f), p);
        m *= glm::mat4_cast(glm::quat(r));
        m = glm::scale(m, s);
    });

    // 부모 월드 행렬 * 내 로컬 행렬 = 내 월드 행렬
    auto update_world = world.system<const TransformMatrix, const TransformMatrix*, TransformMatrix>()
    .kind(flecs::OnUpdate)
    .term_at(0).second<Local>()   // 내 로컬 행렬 (Input)
    .term_at(1).second<World>()   // 부모의 월드 행렬 (Input)
    .term_at(2).second<World>()   // 내 월드 행렬 (Output)

    .term_at(1).parent().cascade() // 부모로부터 가져옴 + 계층 순서 보장
    .each([](flecs::entity entity, const TransformMatrix& local, const TransformMatrix* parent_world, TransformMatrix& world) {
        if (parent_world) 
        {
            // 부모가 있으면: World = ParentWorld * Local
            world=(*parent_world)*local;
        } 
        else 
        {
            // 부모가 없으면(Root): World = Local
            world=local;
        }
    });
}