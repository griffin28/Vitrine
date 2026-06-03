#pragma once

#include <glm/glm.hpp>

#include <memory>

namespace vitrine
{

class Camera;

/// @brief Which projection a Camera implements. Maps 1:1 to an ANARI camera
///        subtype ("perspective" / "orthographic").
enum class CameraType
{
    Perspective,
    Orthographic
};

/// @struct CameraConfig
/// @brief Plain description of a camera's full state.
/// 
/// Used to ferry settings between the renderer (which owns the live Camera) 
/// and the camera configuration dialog. Only the field matching `type` is
/// meaningful for the projection (fovYDegrees for perspective, height for
/// orthographic); both are carried so switching type preserves the other's last value.
struct CameraConfig
{
    CameraType type{CameraType::Perspective};

    glm::vec3 eye{0.0f, 0.0f, 5.0f};
    glm::vec3 center{0.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    float nearClip{0.01f};
    float farClip{1000.0f};

    float fovYDegrees{60.0f}; // perspective
    float height{2.0f};       // orthographic
};

/// @brief Construct the Camera subclass matching `type`.
/// @param type the camera subtype 
/// @return a default-configured camera; callers typically follow with applyConfig().
std::unique_ptr<Camera> makeCamera(CameraType type);

} // namespace vitrine
