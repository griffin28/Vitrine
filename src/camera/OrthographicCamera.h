#pragma once

#include "Camera.h"

namespace vitrine
{

/// @class OrthographicCamera
/// @brief Orthographic ANARI camera.
///
/// OrthographicCamera implements the ANARI "orthographic" camera subtype. It
/// stores the projection height in world units while inheriting shared view
/// state and projection-independent manipulators from Camera.
class OrthographicCamera : public Camera
{
public:
    /// @brief Default constructor
    OrthographicCamera() = default;

    /// @brief Destructor
    ~OrthographicCamera() override = default;

    /// @brief Get the ANARI camera subtype string
    /// @return "orthographic"
    const char* anariSubtype() const override { return "orthographic"; }

    /// @brief Get the camera projection type
    /// @return CameraType::Orthographic
    CameraType type() const override { return CameraType::Orthographic; }

    /// @brief Get the vertical image-plane extent
    /// @return orthographic height in world units
    float height() const { return m_height; }

    /// @brief Set the vertical image-plane extent
    /// @param h orthographic height in world units
    void setHeight(float h) { m_height = h; }

protected:
    /// @brief Set orthographic projection parameters on the ANARI camera
    /// @param device ANARI device that owns the camera
    /// @param camera ANARI camera object to update
    void applyProjection(ANARIDevice device, ANARICamera camera) const override;

    /// @brief Write orthographic projection state to a CameraConfig
    /// @param config camera configuration to update
    void writeProjection(CameraConfig& config) const override;

    /// @brief Read orthographic projection state from a CameraConfig
    /// @param config camera configuration to read
    void readProjection(const CameraConfig& config) override;

private:
    float m_height{2.0f};
};

} // namespace vitrine
