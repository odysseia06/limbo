#include "InspectorPanel.hpp"
#include "EditorApp.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace limbo::editor {

InspectorPanel::InspectorPanel(EditorApp& editor) : m_editor(editor) {}

void InspectorPanel::init() {}

void InspectorPanel::shutdown() {}

void InspectorPanel::render() {
    if (!m_open) {
        return;
    }

    ImGui::Begin("Inspector", &m_open);

    if (m_selectedEntity.isValid()) {
        drawComponents();

        ImGui::Separator();

        // Add component button
        float const buttonWidth = ImGui::GetContentRegionAvail().x;
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
    // ========================================================================
    // IDENTITY (always first, not removable)
    // ========================================================================
    if (m_selectedEntity.hasComponent<NameComponent>()) {
        auto& name = m_selectedEntity.getComponent<NameComponent>();
        drawNameComponent(name);
    }

    // ========================================================================
    // TRANSFORM (core component, usually present)
    // ========================================================================
    if (m_selectedEntity.hasComponent<TransformComponent>()) {
        ImGui::Separator();
        bool const open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##Transform")) {
            m_selectedEntity.removeComponent<TransformComponent>();
        } else if (open) {
            auto& transform = m_selectedEntity.getComponent<TransformComponent>();
            drawTransformComponent(transform);
        }
    }

    // ========================================================================
    // RENDERING COMPONENTS
    // ========================================================================
    bool hasRenderingComponents = m_selectedEntity.hasComponent<SpriteRendererComponent>();

    if (hasRenderingComponents) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Rendering");
        ImGui::Separator();
    }

    // Sprite Renderer
    if (m_selectedEntity.hasComponent<SpriteRendererComponent>()) {
        bool const open =
            ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##SpriteRenderer")) {
            m_selectedEntity.removeComponent<SpriteRendererComponent>();
        } else if (open) {
            auto& sprite = m_selectedEntity.getComponent<SpriteRendererComponent>();
            drawSpriteRendererComponent(sprite);
        }
    }

    // ========================================================================
    // PHYSICS COMPONENTS
    // ========================================================================
    bool hasPhysicsComponents = m_selectedEntity.hasComponent<Rigidbody2DComponent>() ||
                                m_selectedEntity.hasComponent<BoxCollider2DComponent>() ||
                                m_selectedEntity.hasComponent<CircleCollider2DComponent>();

    if (hasPhysicsComponents) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Physics 2D");
        ImGui::Separator();
    }

    // Rigidbody2D
    if (m_selectedEntity.hasComponent<Rigidbody2DComponent>()) {
        bool const open = ImGui::CollapsingHeader("Rigidbody 2D", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##Rigidbody2D")) {
            m_selectedEntity.removeComponent<Rigidbody2DComponent>();
        } else if (open) {
            auto& rb = m_selectedEntity.getComponent<Rigidbody2DComponent>();
            drawRigidbody2DComponent(rb);
        }
    }

    // BoxCollider2D
    if (m_selectedEntity.hasComponent<BoxCollider2DComponent>()) {
        bool const open =
            ImGui::CollapsingHeader("Box Collider 2D", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##BoxCollider2D")) {
            m_selectedEntity.removeComponent<BoxCollider2DComponent>();
        } else if (open) {
            auto& collider = m_selectedEntity.getComponent<BoxCollider2DComponent>();
            drawBoxCollider2DComponent(collider);
        }
    }

    // CircleCollider2D
    if (m_selectedEntity.hasComponent<CircleCollider2DComponent>()) {
        bool const open =
            ImGui::CollapsingHeader("Circle Collider 2D", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##CircleCollider2D")) {
            m_selectedEntity.removeComponent<CircleCollider2DComponent>();
        } else if (open) {
            auto& collider = m_selectedEntity.getComponent<CircleCollider2DComponent>();
            drawCircleCollider2DComponent(collider);
        }
    }

    // ========================================================================
    // SCRIPTING COMPONENTS
    // ========================================================================
    bool hasScriptingComponents = m_selectedEntity.hasComponent<ScriptComponent>();

    if (hasScriptingComponents) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Scripting");
        ImGui::Separator();
    }

    // ScriptComponent
    if (m_selectedEntity.hasComponent<ScriptComponent>()) {
        bool const open = ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::SmallButton("X##Script")) {
            m_selectedEntity.removeComponent<ScriptComponent>();
        } else if (open) {
            auto& script = m_selectedEntity.getComponent<ScriptComponent>();
            drawScriptComponent(script);
        }
    }
}

void InspectorPanel::drawAddComponentMenu() {
    if (ImGui::BeginPopup("AddComponentPopup")) {
        // Core components
        if (!m_selectedEntity.hasComponent<TransformComponent>()) {
            if (ImGui::MenuItem("Transform")) {
                m_selectedEntity.addComponent<TransformComponent>();
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Rendering");

        if (!m_selectedEntity.hasComponent<SpriteRendererComponent>()) {
            if (ImGui::MenuItem("Sprite Renderer")) {
                m_selectedEntity.addComponent<SpriteRendererComponent>(glm::vec4(1.0f));
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Physics 2D");

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

        ImGui::Separator();
        ImGui::TextDisabled("Scripting");

        if (!m_selectedEntity.hasComponent<ScriptComponent>()) {
            if (ImGui::MenuItem("Script")) {
                m_selectedEntity.addComponent<ScriptComponent>();
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

    ImGui::Text("Sorting Layer");
    ImGui::DragInt("##SortingLayer", &component.sortingLayer, 1.0f);

    ImGui::Text("Sorting Order");
    ImGui::DragInt("##SortingOrder", &component.sortingOrder, 1.0f);
}

void InspectorPanel::drawRigidbody2DComponent(Rigidbody2DComponent& component) {
    // Body type
    const char* bodyTypes[] = {"Static", "Kinematic", "Dynamic"};
    int currentType = static_cast<int>(component.type);
    ImGui::Text("Body Type");
    if (ImGui::Combo("##BodyType", &currentType, bodyTypes, 3)) {
        component.type = static_cast<BodyType>(currentType);
    }

    ImGui::Text("Gravity Scale");
    ImGui::DragFloat("##GravityScale", &component.gravityScale, 0.01f, -10.0f, 10.0f);

    ImGui::Checkbox("Fixed Rotation", &component.fixedRotation);

    // Damping (collapsible for less common settings)
    if (ImGui::TreeNode("Damping")) {
        ImGui::Text("Linear");
        ImGui::DragFloat("##LinearDamping", &component.linearDamping, 0.01f, 0.0f, 10.0f);

        ImGui::Text("Angular");
        ImGui::DragFloat("##AngularDamping", &component.angularDamping, 0.01f, 0.0f, 10.0f);

        ImGui::TreePop();
    }
}

void InspectorPanel::drawBoxCollider2DComponent(BoxCollider2DComponent& component) {
    ImGui::Text("Size");
    ImGui::DragFloat2("##Size", glm::value_ptr(component.size), 0.01f, 0.01f, 100.0f);

    ImGui::Text("Offset");
    ImGui::DragFloat2("##BoxOffset", glm::value_ptr(component.offset), 0.01f);

    ImGui::Checkbox("Is Trigger", &component.isTrigger);

    // Material properties (collapsible)
    if (ImGui::TreeNode("Material##Box")) {
        ImGui::Text("Density");
        ImGui::DragFloat("##BoxDensity", &component.density, 0.01f, 0.0f, 100.0f);

        ImGui::Text("Friction");
        ImGui::DragFloat("##BoxFriction", &component.friction, 0.01f, 0.0f, 1.0f);

        ImGui::Text("Restitution");
        ImGui::DragFloat("##BoxRestitution", &component.restitution, 0.01f, 0.0f, 1.0f);

        ImGui::TreePop();
    }
}

void InspectorPanel::drawCircleCollider2DComponent(CircleCollider2DComponent& component) {
    ImGui::Text("Radius");
    ImGui::DragFloat("##CircleRadius", &component.radius, 0.01f, 0.01f, 100.0f);

    ImGui::Text("Offset");
    ImGui::DragFloat2("##CircleOffset", glm::value_ptr(component.offset), 0.01f);

    ImGui::Checkbox("Is Trigger##Circle", &component.isTrigger);

    // Material properties (collapsible)
    if (ImGui::TreeNode("Material##Circle")) {
        ImGui::Text("Density");
        ImGui::DragFloat("##CircleDensity", &component.density, 0.01f, 0.0f, 100.0f);

        ImGui::Text("Friction");
        ImGui::DragFloat("##CircleFriction", &component.friction, 0.01f, 0.0f, 1.0f);

        ImGui::Text("Restitution");
        ImGui::DragFloat("##CircleRestitution", &component.restitution, 0.01f, 0.0f, 1.0f);

        ImGui::TreePop();
    }
}

void InspectorPanel::drawScriptComponent(ScriptComponent& component) {
    // Script file path display and picker
    ImGui::Text("Script File");

    String displayPath = component.scriptPath.empty() ? "(None)" : component.scriptPath.string();

    // Truncate path for display if too long
    if (displayPath.length() > 40) {
        displayPath = "..." + displayPath.substr(displayPath.length() - 37);
    }

    ImGui::PushItemWidth(-60);
    ImGui::InputText("##ScriptPath", displayPath.data(), displayPath.size(),
                     ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("...##SelectScript")) {
        ImGui::OpenPopup("ScriptFilePicker");
    }

    drawScriptFilePicker(component);

    // Status indicator
    ImGui::Text("Status");
    if (component.scriptPath.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No script assigned");
    } else if (component.hasError()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error");
    } else if (!component.enabled) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Disabled");
    } else if (component.initialized) {
        if (component.started) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Running");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Initialized");
        }
    } else {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Pending");
    }

    // Show error details if there's an error
    if (component.hasError()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        if (component.lastErrorLine > 0) {
            ImGui::Text("Line %d: %s", component.lastErrorLine, component.lastError.c_str());
        } else {
            ImGui::TextWrapped("%s", component.lastError.c_str());
        }
        ImGui::PopStyleColor();
    }

    // Enabled checkbox
    ImGui::Checkbox("Enabled##Script", &component.enabled);

    // Show available callbacks (read-only info)
    if (component.initialized && ImGui::TreeNode("Callbacks")) {
        auto showCallback = [](const char* name, bool valid) {
            if (valid) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[+] %s", name);
            } else {
                ImGui::TextDisabled("[-] %s", name);
            }
        };

        showCallback("onStart", component.onStart.valid());
        showCallback("onUpdate", component.onUpdate.valid());
        showCallback("onDestroy", component.onDestroy.valid());
        showCallback("onCollisionBegin", component.onCollisionBegin.valid());
        showCallback("onCollisionEnd", component.onCollisionEnd.valid());
        showCallback("onTriggerEnter", component.onTriggerEnter.valid());
        showCallback("onTriggerExit", component.onTriggerExit.valid());

        ImGui::TreePop();
    }
}

void InspectorPanel::drawScriptFilePicker(ScriptComponent& component) {
    if (ImGui::BeginPopup("ScriptFilePicker")) {
        ImGui::Text("Select Script");
        ImGui::Separator();

        // Look for .lua files in assets/scripts/
        std::filesystem::path const scriptsDir =
            std::filesystem::current_path() / "assets" / "scripts";

        if (std::filesystem::exists(scriptsDir)) {
            bool foundAny = false;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptsDir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                    foundAny = true;
                    // Get relative path from scripts dir
                    auto relativePath = std::filesystem::relative(entry.path(), scriptsDir);
                    String const displayName = relativePath.string();

                    bool const isSelected = (entry.path() == component.scriptPath);
                    if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                        component.scriptPath = entry.path();
                        // Reset state so script will be reloaded
                        component.initialized = false;
                        component.started = false;
                        component.enabled = true;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            if (!foundAny) {
                ImGui::TextDisabled("No .lua files found in assets/scripts/");
            }
        } else {
            ImGui::TextDisabled("Directory not found: assets/scripts/");
        }

        ImGui::Separator();
        if (ImGui::Selectable("(Clear)")) {
            component.scriptPath.clear();
            component.initialized = false;
            component.started = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

}  // namespace limbo::editor
