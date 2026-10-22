#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace vitrine
{

/// @brief Which light a LightConfig describes. Maps 1:1 to an ANARI light
///        subtype ("directional" / "point" / "spot").
enum class LightType
{
    Directional,
    Point,
    Spot
};

/// @struct LightConfig
/// @brief Plain description of one scene light.
///
/// Used to ferry settings between the renderer (which owns the ANARI light
/// handles), the light dialog, the gizmo overlay, and QSettings. Only the
/// fields relevant to `type` are sent to ANARI; the rest are carried so that
/// switching type preserves the user's last values.
struct LightConfig
{
    LightType type{LightType::Directional};
    bool enabled{true};

    glm::vec3 color{1.0f, 1.0f, 1.0f};

    glm::vec3 position{0.0f, 2.0f, 2.0f};     // point, spot
    glm::vec3 direction{-1.0f, -1.0f, -1.0f}; // directional, spot: direction of travel

    /// Directional: irradiance (W/m^2). Point / spot: intensity (W/sr).
    float intensity{3.0f};

    float openingAngleDegrees{60.0f}; // spot: full cone angle
    float falloffAngleDegrees{5.0f};  // spot: smooth region inside the cone edge
};

/// @brief Maximum number of lights the renderer sends to the backend.
///        Phenocryst uploads at most 16; extras are dropped with a warning.
constexpr size_t kMaxLights = 16;

/// @brief The lights used before the user configures any: a single directional
///        light, because Phenocryst has no headlight and renders unlit
///        (ambient only) without at least one light.
inline std::vector<LightConfig> defaultLightConfigs()
{
    return {LightConfig{}};
}

} // namespace vitrine
