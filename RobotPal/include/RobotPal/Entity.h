#ifndef __ENTITY_H__
#define __ENTITY_H__
#include <flecs.h>

class Entity 
{
public:
    Entity() = default;
    Entity(flecs::entity handle);
    Entity(const Entity& other) = default;

    template<typename T>
    bool HasComponent() const
    {
        return m_EntityHandle.has<T>();
    }

    template<typename T, typename... Args>
    void AddComponent(Args&&... args)
    {
        m_EntityHandle.add<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    void RemoveComponent()
    {
        m_EntityHandle.remove<T>();
    }

    template<typename T>
    T* GetComponent()
    {
        return m_EntityHandle.get_mut<T>();
    }

    template<typename T>
    const T* GetComponent() const
    {
        return m_EntityHandle.get<T>();
    }

    template<typename T>
    void SetComponent(const T& component)
    {
        m_EntityHandle.set<T>(component);
    }

    flecs::entity GetHandle() const { return m_EntityHandle; }

    bool IsValid() const 
    {
        return m_EntityHandle.is_valid();
    }

    operator flecs::entity() const { return m_EntityHandle; }
    operator uint64_t() const { return m_EntityHandle.id(); }

    bool operator==(const Entity& other) const
    {
        return m_EntityHandle == other.m_EntityHandle;
    }
    
    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }


private:
    flecs::entity m_EntityHandle { flecs::entity::null() };
};

#endif