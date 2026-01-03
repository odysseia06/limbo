#include "InspectorPanel.hpp"
#include "EditorApp.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace limbo::editor {

InspectorPanel::InspectorPanel(EditorApp& editor)
    : m_editor(editor)
{
}

void InspectorPanel::init() {
}

void InspectorPanel::shutdown() {
}

void InspectorPanel::render() {
    if (!m_open) return;

    ImGui::Begin("Inspector", &m_open);

    if (m_selectedEntity.isValid()) {
        drawComponents();
        
        ImGui::Separator();
        
        // Add component button
        float buttonWidth = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0))) {
            ImGui::OpenPopup("AddComponentPopup");
        }
        
        drawAddComponentMenu();
    } else {
        ImGui::TextDisabled("No entity selected");
    }

    ImGui::End();
}

void InspectorPanel::drawComponents() {
    // Name component (always present, not removable)
    if (m_selectedEntity.hasComponent<NameComponent>()) {
        auto& name = m_selectedEntity.getComponent<NameComponent>();
        drawNameComponent(name);
    }

    // Transform component
    if (m_selectedEntity.hasComponent<TransformComponent>()) {
        ImGui::Separator();
        bool open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);
        
        // Remove button
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##Transform")) {
            m_selectedEntity.removeComponent<TransformComponent>();
        } else if (open) {
            auto& transform = m_selectedEntity.getComponent<TransformComponent>();
            drawTransformComponent(transform);
        }
    }

    // Sprite Renderer component
    if (m_selectedEntity.hasComponent<SpriteRendererComponent>()) {
        ImGui::Separator();
        bool open = ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen);
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##SpriteRenderer")) {
            m_selectedEntity.removeComponent<SpriteRendererComponent>();
        } else if (open) {
            auto& sprite = m_selectedEntity.getComponent<SpriteRendererComponent>();
            drawSpriteRendererComponent(sprite);
        }
    }

    // Rigidbody2D component
    if (m_selectedEntity.hasComponent<Rigidbody2DComponent>()) {
        ImGui::Separator();
        bool open = ImGui::CollapsingHeader("Rigidbody 2D", ImGuiTreeNodeFlags_DefaultOpen);
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##Rigidbody2D")) {
            m_selectedEntity.removeComponent<Rigidbody2DComponent>();
        } else if (open) {
            auto& rb = m_selectedEntity.getComponent<Rigidbody2DComponent>();
            drawRigidbody2DComponent(rb);
        }
    }

    // BoxCollider2D component
    if (m_selectedEntity.hasComponent<BoxCollider2DComponent>()) {
        ImGui::Separator();
        bool open = ImGui::CollapsingHeader("Box Collider 2D", ImGuiTreeNodeFlags_DefaultOpen);
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##BoxCollider2D")) {
            m_selectedEntity.removeComponent<BoxCollider2DComponent>();
        } else if (open) {
            auto& collider = m_selectedEntity.getComponent<BoxCollider2DComponent>();
            drawBoxCollider2DComponent(collider);
        }
    }

    // CircleCollider2D component
    if (m_selectedEntity.hasComponent<CircleCollider2DComponent>()) {
        ImGui::Separator();
        bool open = ImGui::CollapsingHeader("Circle Collider 2D", ImGuiTreeNodeFlags_DefaultOpen);
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##CircleCollider2D")) {
            m_selectedEntity.removeComponent<CircleCollider2DComponent>();
        } else if (open) {
            auto& collider = m_selectedEntity.getComponent<CircleCollider2DComponent>();
            drawCircleCollider2DComponent(collider);
        }
    }
}

void InspectorPanel::drawAddComponentMenu() {
    if (ImGui::BeginPopup("AddComponentPopup")) {
        if (!m_selectedEntity.hasComponent<TransformComponent>()) {
            if (ImGui::MenuItem("Transform")) {
                m_selectedEntity.addComponent<TransformComponent>();
                ImGui::CloseCurrentPopup();
            }
        }
        if (!m_selectedEntity.hasComponent<SpriteRendererComponent>()) {
            if (ImGui::MenuItem("Sprite Renderer")) {
                m_selectedEntity.addComponent<SpriteRendererComponent>(glm::vec4(1.0f));
                ImGui::CloseCurrentPopup();
            }
        }
        if (!m_selectedEntity.hasComponent<Rigidbody2DComponent>()) {
            if (ImGui::MenuItem("Rigidbody 2D")) {
                m_selectedEntity.addComponent<Rigidbody2DComponent>(BodyType::Dynamic);
                ImGui::CloseCurrentPopup();
            }
        }
        if (!m_selectedEntity.hasComponent<BoxCollider2DComponent>()) {
            if (ImGui::MenuItem("Box Collider 2D")) {
                m_selectedEntity.addComponent<BoxCollider2DComponent>(glm::vec2(0.5f));
                ImGui::CloseCurrentPopup();
            }
        }
        if (!m_selectedEntity.hasComponent<CircleCollider2DComponent>()) {
            if (ImGui::MenuItem("Circle Collider 2D")) {
                m_selectedEntity.addComponent<CircleCollider2DComponent>(0.5f);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void InspectorPanel::drawNameComponent(NameComponent& component) {
    char buffer[256];
    std::strncpy(buffer, component.name.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    ImGui::PushItemWidth(-1);
    if (ImGui::InputText("##Name", buffer, sizeof(buffer))) {
        component.name = buffer;
    }
    ImGui::PopItemWidth();
}

void InspectorPanel::drawTransformComponent(TransformComponent& component) {
    ImGui::PushItemWidth(-1);

    // Position
    ImGui::Text("Position");
    ImGui::DragFloat3("##Position", glm::value_ptr(component.position), 0.01f);

    // Rotation (convert to degrees for display)
    ImGui::Text("Rotation");
    glm::vec3 rotationDegrees = glm::degrees(component.rotation);
    if (ImGui::DragFloat3("##Rotation", glm::value_ptr(rotationDegrees), 1.0f)) {
        component.rotation = glm::radians(rotationDegrees);
    }

    // Scale
    ImGui::Text("Scale");
    ImGui::DragFloat3("##Scale", glm::value_ptr(component.scale), 0.01f, 0.001f, 100.0f);

    ImGui::PopItemWidth();
}

void InspectorPanel::drawSpriteRendererComponent(SpriteRendererComponent& component) {
    ImGui::Text("Color");
    ImGui::ColorEdit4("##Color", glm::value_ptr(component.color));

    // TODO: Texture selector
    ImGui::Text("Texture");
    ImGui::Button("None (Select)", ImVec2(-1, 0));
}

void InspectorPanel::drawRigidbody2DComponent(Rigidbody2DComponent& component) {
    // Body type
    const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
    int currentType = static_cast<int>(component.type);
    ImGui::Text("Body Type");
    if (ImGui::Combo("##BodyType", &currentType, bodyTypes, 3)) {
        component.type = static_cast<BodyType>(currentType);
    }

    ImGui::Checkbox("Fixed Rotation", &component.fixedRotation);
}

void InspectorPanel::drawBoxCollider2DComponent(BoxCollider2DComponent& component) {
    ImGui::Text("Size");
    ImGui::DragFloat2("##Size", glm::value_ptr(component.size), 0.01f, 0.01f, 100.0f);

    ImGui::Text("Offset");
    ImGui::DragFloat2("##Offset", glm::value_ptr(component.offset), 0.01f);

    ImGui::Text("Density");
    ImGui::DragFloat("##Density", &component.density, 0.01f, 0.0f, 100.0f);

    ImGui::Text("Friction");
    ImGui::DragFloat("##Friction", &component.friction, 0.01f, 0.0f, 1.0f);

    ImGui::Text("Restitution");
    ImGui::DragFloat("##Restitution", &component.restitution, 0.01f, 0.0f, 1.0f);
}

void InspectorPanel::drawCircleCollider2DComponent(CircleCollider2DComponent& component) {
    ImGui::Text("Radius");
    ImGui::DragFloat("##Radius", &component.radius, 0.01f, 0.01f, 100.0f);

    ImGui::Text("Offset");
    ImGui::DragFloat2("##Offset", glm::value_ptr(component.offset), 0.01f);

    ImGui::Text("Density");
    ImGui::DragFloat("##Density", &component.density, 0.01f, 0.0f, 100.0f);

    ImGui::Text("Friction");
    ImGui::DragFloat("##Friction", &component.friction, 0.01f, 0.0f, 1.0f);

    ImGui::Text("Restitution");
    ImGui::DragFloat("##Restitution", &component.restitution, 0.01f, 0.0f, 1.0f);
}

} // namespace limbo::editor
