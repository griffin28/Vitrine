#pragma once

#include "Camera.h"

#include <glm/trigonometric.hpp>

namespace vitrine
{

/// @class PerspectiveCamera
/// @brief Perspective ANARI camera.
///
/// PerspectiveCamera implements the ANARI "perspective" camera subtype. It
/// stores the vertical field of view while inheriting shared view state and
/// projection-independent manipulators from Camera.
class PerspectiveCamera : public Camera
{
public:
    /// @brief Default constructor
    PerspectiveCamera() = default;

    /// @brief Destructor
    ~PerspectiveCamera() override = default;

    /// @brief Get the ANARI camera subtype string
    /// @return "perspective"
    const char* anariSubtype() const override { return "perspective"; }

    /// @brief Get the camera projection type
    /// @return CameraType::Perspective
    CameraType type() const override { return CameraType::Perspective; }

    /// @brief Get the vertical field of view
    /// @return vertical field of view in radians
    float fovY() const { return m_fovY; }

    /// @brief Set the vertical field of view
    /// @param radians vertical field of view in radians
    void setFovY(float radians) { m_fovY = radians; }

protected:
    /// @brief Set perspective projection parameters on the ANARI camera
    /// @param device ANARI device that owns the camera
    /// @param camera ANARI camera object to update
    void applyProjection(ANARIDevice device, ANARICamera camera) const override;

    /// @brief Write perspective projection state to a CameraConfig
    /// @param config camera configuration to update
    void writeProjection(CameraConfig& config) const override;

    /// @brief Read perspective projection state from a CameraConfig
    /// @param config camera configuration to read
    void readProjection(const CameraConfig& config) override;

private:
    float m_fovY{glm::radians(60.0f)};
};

} // namespace vitrine
