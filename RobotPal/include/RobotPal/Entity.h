#ifndef __ENTITY_H__
#define __ENTITY_H__

#include <flecs.h>
#include <cassert>
#include <string>
#include <vector>
#include <glm/glm.hpp>
class Entity 
{
public:
    Entity() = default;
    Entity(flecs::entity handle);
    Entity(const Entity& other) = default;

    // -------------------------------------------------------
    // Unity-like Hierarchy (Transform)
    // -------------------------------------------------------

    glm::vec3 GetLocalPosition();
    glm::vec3 GetLocalRotation();
    glm::vec3 GetLocalScale();

    void SetLocalPosition(const glm::vec3& pos);
    void SetLocalRotation(const glm::vec3& rot);
    void SetLocalScale(const glm::vec3& scale);

    Entity GetParent() const;

    Entity GetChild(const std::string& name) const;

    //모든 직속 자식 가져오기 (transform.childCount + loop)
    std::vector<Entity> GetChildren() const;

    //자식들 중에서 특정 컴포넌트 찾기 (GetComponentInChildren<T>)
    //recursive: true면 손자, 증손자까지 뒤짐 (DFS)
    template <typename T>
    Entity FindChildWith(bool recursive = true) const {
        if (!IsValid()) return Entity();

        Entity found;
        
        // Flecs의 BFS/DFS 순회 기능 활용 (filter + parent 관계)
        // 여기서는 간단히 children 콜백으로 구현 예시
        FindChildWithRecursive<T>(m_EntityHandle, found, recursive);
        
        return found;
    }

    // -----------------------------------------------------------------------
    // 1. 쓰기 및 생성 (Fluent Interface - 체이닝 지원)
    // -----------------------------------------------------------------------

    // 값 설정 (복사/이동) - 초기화에 적합
    // 예: entity.Set<Position>({10, 20});
    template<typename T>
    Entity& Set(const T& component)
    {
        m_EntityHandle.set<T>(component);
        return *this;
    }

    // 태그 추가 (데이터 없는 컴포넌트)
    // 예: entity.AddTag<PlayerTag>();
    template<typename T>
    Entity& AddTag()
    {
        m_EntityHandle.add<T>();
        return *this;
    }

    // 컴포넌트 제거
    template<typename T>
    Entity& Remove()
    {
        m_EntityHandle.remove<T>();
        return *this;
    }

    // -----------------------------------------------------------------------
    // 2. 수정 (Mutable Access - 변경 감지 작동)
    // -----------------------------------------------------------------------

    // [참조 반환] 컴포넌트가 없으면 생성, 있으면 가져옴 (가장 추천하는 수정 방식)
    // 예: entity.Ensure<Position>().x += 5;
    template<typename T>
    T& Get()
    {
        return m_EntityHandle.get_mut<T>();
    }

    // [포인터 반환] 컴포넌트 수정 권한 획득 (외부 API 전달용)
    // 예: ImGui::InputFloat("X", &entity.GetMut<Position>()->x);
    template<typename T>
    T* GetPtr()
    {
        return &m_EntityHandle.get_mut<T>();
    }

    // 존재 여부 확인
    template<typename T>
    bool Has() const
    {
        return m_EntityHandle.has<T>();
    }

    // -----------------------------------------------------------------------
    // 4. 계층 구조 (Hierarchy)
    // -----------------------------------------------------------------------

    Entity& SetParent(Entity parent)
    {
        m_EntityHandle.child_of(parent.GetHandle());
        return *this;
    }

    Entity& AddChild(Entity child)
    {
        child.GetHandle().child_of(m_EntityHandle);
        return *this;
    }

    // -----------------------------------------------------------------------
    // 5. 유틸리티 및 연산자
    // -----------------------------------------------------------------------

    flecs::entity GetHandle() const { return m_EntityHandle; }

    bool IsValid() const 
    {
        return m_EntityHandle.is_alive(); // is_valid보다 is_alive가 더 정확한 표현일 때가 많음 (상황에 따라 선택)
    }

    void Destroy()
    {
        if (IsValid()) m_EntityHandle.destruct();
    }

    // 암시적 형변환
    operator flecs::entity() const { return m_EntityHandle; }
    operator uint64_t() const { return m_EntityHandle.id(); }
    operator bool() const { return IsValid(); } // if (entity) ... 지원

    bool operator==(const Entity& other) const
    {
        return m_EntityHandle == other.m_EntityHandle;
    }
    
    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

private:
    template <typename T>
    void FindChildWithRecursive(flecs::entity current, Entity& result, bool recursive) const {
        if (result.IsValid()) return; // 이미 찾았으면 중단

        current.children([&](flecs::entity child) {
            if (result.IsValid()) return;

            if (child.has<T>()) {
                result = Entity(child);
                return;
            }

            if (recursive) {
                FindChildWithRecursive<T>(child, result, recursive);
            }
        });
    }

    flecs::entity m_EntityHandle { flecs::entity::null() };
};

#endif