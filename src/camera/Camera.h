#pragma once

#include <anari/anari.h>

#include <QVector3D>

#include <glm/glm.hpp>

namespace vitrine
{

/// @brief Abstract camera base. Owns the *view* state (eye / center / up,
///        aspect, clip planes) and all projection-independent manipulators
///        (orbit / pan / dolly). Projection-specific state lives in the
///        derived class, which advertises its ANARI subtype and pushes its
///        own parameters via applyProjection().
///
/// This split is what makes a future OrthographicCamera a drop-in: it would
/// only override anariSubtype() / applyProjection() (setting "height" instead
/// of "fovy") and reuse every manipulator here unchanged.
class Camera
{
public:
    Camera() = default;
    virtual ~Camera() = default;

    Camera(const Camera&) = default;
    Camera& operator=(const Camera&) = default;

    /// @brief ANARI camera subtype string (e.g. "perspective").
    virtual const char* anariSubtype() const = 0;

    /// @brief Push shared view parameters (position/direction/up/aspect/
    ///        near/far) plus the projection-specific ones, then commit.
    void commit(ANARIDevice device, ANARICamera camera) const;

    // --- View manipulators (projection-independent) -----------------------

    /// @brief Orbit the eye around the center. Angles in radians; pitch is
    ///        clamped to avoid flipping over the poles.
    void orbit(float yawRadians, float pitchRadians);

    /// @brief Translate eye and center together within the view plane.
    ///        Deltas are in world units (already scaled by the caller).
    void pan(float dx, float dy);

    /// @brief Move the eye toward (positive) or away from (negative) the
    ///        center along the view direction. Distance is clamped to a small
    ///        minimum so the eye never reaches the center.
    void dolly(float amount);

    // --- Accessors --------------------------------------------------------

    void setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
    void setAspect(float aspect) { m_aspect = aspect; }
    void setMinEyeDistance(float d) { m_minEyeDistance = d; }

    glm::vec3 eye() const { return m_eye; }
    glm::vec3 center() const { return m_center; }

    /// @brief Distance from eye to center.
    float distance() const;

    // Orthonormal view basis, returned as QVector3D so callers (e.g. the
    // axis overlay) don't need glm. forward points from eye toward center.
    QVector3D right() const;
    QVector3D upVector() const;
    QVector3D forward() const;

protected:
    /// @brief Set the projection-specific parameters on the ANARI camera
    ///        (no commit; commit() handles that).
    virtual void applyProjection(ANARIDevice device, ANARICamera camera) const = 0;

    // Computed orthonormal basis from the current eye/center/up.
    glm::vec3 forwardVec() const;
    glm::vec3 rightVec() const;
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
