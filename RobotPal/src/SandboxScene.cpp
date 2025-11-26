#include "RobotPal/SandboxScene.h"
#include "RobotPal/Buffer.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Components/Components.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp> // [중요] 쿼터니언 -> 행렬 변환용
#include <imgui.h>
#include <glad/gles2.h>
#include <GLFW/glfw3.h>
#include <iostream>

void SandboxScene::OnEnter()
{
    auto modelPrefab = AssetManager::Get().GetPrefab(m_World, "./Assets/jetank.glb");
    auto prefabEntity = CreateEntity("mainModel");
    prefabEntity.GetHandle().is_a(modelPrefab);
}

void SandboxScene::OnUpdate(float dt)
{

}

void SandboxScene::OnExit()
{

}
void SandboxScene::OnImGuiRender()
{
    ImGui::Begin("MainModel Hierarchy"); // 창 이름 변경

    // 1. 선택된 엔티티 상태 (static 또는 멤버 변수)
    static flecs::entity selectedEntity;

    // 2. 재귀 람다 (이전과 동일)
    auto drawNode = [&](auto&& self, flecs::entity e) -> void {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (selectedEntity == e) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool hasChildren = false;
        e.children([&](flecs::entity) { hasChildren = true; });
        if (!hasChildren) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        // mainModel을 루트로 보여줄 때는 처음에 트리를 펼쳐두면 보기 좋습니다. (선택사항)
        if (e.name() == "mainModel") {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        const char* name = e.name().c_str();
        std::string label = (name && *name) ? name : std::to_string(e.id());

        bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)e.id(), flags, "%s", label.c_str());

        if (ImGui::IsItemClicked()) {
            selectedEntity = e;
        }

        if (opened) {
            if (hasChildren) {
                e.children([&](flecs::entity child) {
                    self(self, child);
                });
            }
            ImGui::TreePop();
        }
    };

    // 3. 'mainModel' 찾아서 그리기 (변경된 부분)
    flecs::entity rootModel = m_SceneRoot;

    if (rootModel.is_alive()) {
        // 찾았으면 이 엔티티를 루트로 트리 그리기 시작
        rootModel.children([&](flecs::entity child) {
            drawNode(drawNode, child);
        });
        //drawNode(drawNode, rootModel);
    } else {
        // 아직 로딩이 안 됐거나 이름이 틀린 경우
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "'mainModel' not found.");
    }

    ImGui::End();
}