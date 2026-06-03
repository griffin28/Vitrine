#pragma once

#include <anari/anari.h>

#include <QVector3D>

#include <glm/glm.hpp>

#include "CameraConfig.h"

namespace vitrine
{
/// @class Camera
/// @brief Abstract ANARI camera base.
///
/// Camera owns the shared view state for an ANARI camera, including eye,
/// center, up, aspect, clip planes, and projection-independent manipulators
/// such as orbit, pan, and dolly.
///
/// Projection-specific state lives in derived classes. Each derived class
/// reports its ANARI camera subtype and writes its own projection parameters
/// through applyProjection().
class Camera
{
public:
    /// @brief Default constructor
    Camera() = default;

    /// @brief Destructor
    virtual ~Camera() = default;

    /// @brief Copy constructor
    Camera(const Camera&) = default;

    /// @brief Copy assignment operator
    /// @return reference to this camera
    Camera& operator=(const Camera&) = default;

    /// @brief Get the ANARI camera subtype string
    /// @return ANARI camera subtype string, such as "perspective"
    virtual const char* anariSubtype() const = 0;

    /// @brief Get the camera projection type
    /// @return projection type implemented by this camera
    virtual CameraType type() const = 0;

    /// @brief Commit camera state to an ANARI camera
    /// @param device ANARI device that owns the camera
    /// @param camera ANARI camera object to update
    void commit(ANARIDevice device, ANARICamera camera) const;

    /// @brief Convert the current camera state to a CameraConfig
    /// @return camera configuration containing view and projection state
    CameraConfig toConfig() const;

    /// @brief Apply a CameraConfig to this camera
    /// @param config camera configuration containing view and projection state
    void applyConfig(const CameraConfig& config);

    /// @brief Orbit the eye around the center
    /// @param yawRadians yaw angle in radians
    /// @param pitchRadians pitch angle in radians
    void orbit(float yawRadians, float pitchRadians);

    /// @brief Translate the eye and center together in the view plane
    /// @param dx horizontal pan distance in world units
    /// @param dy vertical pan distance in world units
    void pan(float dx, float dy);

    /// @brief Move the eye toward or away from the center
    /// @param amount distance to move along the view direction
    void dolly(float amount);

    /// @brief Set the look-at camera view
    /// @param eye camera eye position
    /// @param center look-at center position
    /// @param up camera up vector
    void setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

    /// @brief Set the viewport aspect ratio
    /// @param aspect viewport width divided by height
    void setAspect(float aspect) { m_aspect = aspect; }

    /// @brief Set the minimum allowed eye-to-center distance
    /// @param d minimum eye distance
    void setMinEyeDistance(float d) { m_minEyeDistance = d; }

    /// @brief Get the near clipping plane
    /// @return near clipping distance
    float nearClip() const { return m_near; }

    /// @brief Get the far clipping plane
    /// @return far clipping distance
    float farClip() const { return m_far; }

    /// @brief Set the near clipping plane
    /// @param n near clipping distance
    void setNearClip(float n) { m_near = n; }

    /// @brief Set the far clipping plane
    /// @param f far clipping distance
    void setFarClip(float f) { m_far = f; }

    /// @brief Get the camera eye position
    /// @return camera eye position
    glm::vec3 eye() const { return m_eye; }

    /// @brief Get the camera look-at center
    /// @return camera look-at center
    glm::vec3 center() const { return m_center; }

    /// @brief Get the distance from eye to center
    /// @return distance from eye to center
    float distance() const;

    /// @brief Get the camera right basis vector
    /// @return camera right basis vector
    QVector3D right() const;

    /// @brief Get the camera up basis vector
    /// @return camera up basis vector
    QVector3D upVector() const;

    /// @brief Get the camera forward basis vector
    /// @return camera forward basis vector from eye toward center
    QVector3D forward() const;

protected:
    /// @brief Set projection-specific parameters on the ANARI camera
    /// @param device ANARI device that owns the camera
    /// @param camera ANARI camera object to update
    virtual void applyProjection(ANARIDevice device, ANARICamera camera) const = 0;

    /// @brief Write projection-specific state to a CameraConfig
    /// @param config camera configuration to update
    virtual void writeProjection(CameraConfig& config) const = 0;

    /// @brief Read projection-specific state from a CameraConfig
    /// @param config camera configuration to read
    virtual void readProjection(const CameraConfig& config) = 0;

    /// @brief Compute the forward basis vector
    /// @return forward basis vector from eye toward center
    glm::vec3 forwardVec() const;

    /// @brief Compute the right basis vector
    /// @return right basis vector
    glm::vec3 rightVec() const;

    /// @brief Compute the orthonormal up basis vector
    /// @return orthonormal up basis vector
    glm::vec3 trueUpVec() const;

    glm::vec3 m_eye{0.0f, 0.0f, 5.0f};
    glm::vec3 m_center{0.0f, 0.0f, 0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};

    float m_aspect{1.0f};
    float m_near{0.01f};
    float m_far{1000.0f};
    float m_minEyeDistance{0.05f};
};

} // namespace vitrine
