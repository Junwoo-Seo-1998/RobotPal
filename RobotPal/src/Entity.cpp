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