#include "RobotPal/Entity.h"

Entity::Entity(flecs::entity handle)
    : m_EntityHandle(handle)
{
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