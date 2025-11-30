#include "RobotPal/Entity.h"
#include "RobotPal/Components/Components.h"
Entity::Entity(flecs::entity handle)
    : m_EntityHandle(handle)
{
}

glm::vec3 Entity::GetLocalPosition()
{
    return m_EntityHandle.get_mut<Position, Local>();
}

glm::vec3 Entity::GetLocalRotation()
{
    return m_EntityHandle.get_mut<Rotation, Local>();
}

glm::vec3 Entity::GetLocalScale()
{
    return m_EntityHandle.get_mut<Scale, Local>();
}

void Entity::SetLocalPosition(const glm::vec3 &pos)
{
    m_EntityHandle.set<Position, Local>({pos});
}

void Entity::SetLocalRotation(const glm::vec3 &rot)
{
    m_EntityHandle.set<Rotation, Local>({rot});
}

void Entity::SetLocalScale(const glm::vec3 &scale)
{
    m_EntityHandle.set<Scale, Local>({scale});
}

Entity Entity::GetParent() const
{
    return m_EntityHandle.parent();
}

Entity Entity::GetChild(const std::string &name) const
{
    if (!IsValid())
        return Entity();
    // lookup은 기본적으로 자식(또는 경로)에서 찾습니다.
    return Entity(m_EntityHandle.lookup(name.c_str()));
}

std::vector<Entity> Entity::GetChildren() const
{
    std::vector<Entity> children;
    if (IsValid())
    {
        m_EntityHandle.children([&](flecs::entity child)
                                { children.push_back(Entity(child)); });
    }
    return children;
}
Entity Entity::FindChildByNameRecursive(flecs::entity parent, const std::string &targetName)
{
    flecs::entity found = flecs::entity::null();

    // parent의 모든 직계 자식을 순회
    parent.children([&](flecs::entity child)
                    {
        // 이미 찾았다면 더 이상 수행하지 않고 리턴
        if (found != flecs::entity::null()) return;

        // 1. 이름 확인
        if (child.name().c_str() == targetName) {
            found = child;
            return;
        }

        // 2. 못 찾았다면, 해당 자식의 자식들을 재귀적으로 탐색
        flecs::entity res = FindChildByNameRecursive(child, targetName);
        if (res != flecs::entity::null()) {
            found = res;
        } });

    return found;
}