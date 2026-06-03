#pragma once

#include "Camera.h"

#include <glm/trigonometric.hpp>

namespace vitrine
{

/// @brief A pinhole perspective camera (ANARI subtype "perspective"). Adds a
///        vertical field of view to the shared view state; everything else
///        (orbit/pan/dolly, position/direction/up) comes from Camera.
class PerspectiveCamera : public Camera
{
public:
    PerspectiveCamera() = default;
    ~PerspectiveCamera() override = default;

    const char* anariSubtype() const override { return "perspective"; }

    /// @brief Vertical field of view in radians.
    float fovY() const { return m_fovY; }
    void setFovY(float radians) { m_fovY = radians; }

protected:
    void applyProjection(ANARIDevice device, ANARICamera camera) const override;

private:
    float m_fovY{glm::radians(60.0f)};
};

} // namespace vitrine
