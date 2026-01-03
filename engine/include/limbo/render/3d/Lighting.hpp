#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/ecs/System.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace limbo {

/**
 * @brief Directional light (sun-like, infinite distance)
 */
struct DirectionalLight {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    bool castShadows = true;
};

/**
 * @brief Point light (omnidirectional, local)
 */
struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;

    // Attenuation factors
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    bool castShadows = false;
};

/**
 * @brief Spot light (cone-shaped, local)
 */
struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerCutoff = 12.5f;  // degrees
    float outerCutoff = 17.5f;  // degrees

    // Attenuation factors
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    bool castShadows = false;
};

/**
 * @brief Ambient light settings
 */
struct AmbientLight {
    glm::vec3 color{0.1f};
    float intensity = 1.0f;
};

/**
 * @brief Light component for entities
 */
struct LightComponent {
    enum class Type { Directional, Point, Spot };

    Type type = Type::Point;
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;          // Point/Spot
    float innerCutoff = 12.5f;    // Spot only
    float outerCutoff = 17.5f;    // Spot only
    bool castShadows = false;

    // Attenuation (Point/Spot)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
};

/**
 * @brief Maximum lights supported in forward rendering
 */
constexpr u32 MAX_DIRECTIONAL_LIGHTS = 4;
constexpr u32 MAX_POINT_LIGHTS = 64;
constexpr u32 MAX_SPOT_LIGHTS = 32;

/**
 * @brief Lighting environment containing all active lights
 */
struct LightingEnvironment {
    AmbientLight ambient;
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;

    void clear();
    u32 getTotalLightCount() const;
};

/**
 * @brief System that gathers lights from entities for rendering
 */
class LightingSystem : public System {
public:
    LightingSystem() = default;
    ~LightingSystem() override = default;

    void onAttach(World& world) override;
    void onDetach() override;
    void update(float deltaTime) override;

    /// Set ambient light
    void setAmbientLight(const glm::vec3& color, float intensity = 1.0f);

    /// Get the current lighting environment
    const LightingEnvironment& getLightingEnvironment() const { return m_environment; }

    /// Upload lighting data to shader uniforms
    void uploadToShader(Shader& shader) const;

private:
    World* m_world = nullptr;
    LightingEnvironment m_environment;
};

}  // namespace limbo
