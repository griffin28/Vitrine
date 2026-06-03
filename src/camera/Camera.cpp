#include "Camera.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace vitrine
{
glm::vec3 Camera::forwardVec() const
{
    const glm::vec3 d = m_center - m_eye;
    const float len = glm::length(d);
    return len > 0.0f ? d / len : glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 Camera::rightVec() const
{
    const glm::vec3 r = glm::cross(forwardVec(), m_up);
    const float len = glm::length(r);
    return len > 0.0f ? r / len : glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 Camera::trueUpVec() const
{
    return glm::normalize(glm::cross(rightVec(), forwardVec()));
}

float Camera::distance() const
{
    return glm::length(m_center - m_eye);
}

QVector3D Camera::right() const
{
    const glm::vec3 v = rightVec();
    return QVector3D(v.x, v.y, v.z);
}

QVector3D Camera::upVector() const
{
    const glm::vec3 v = trueUpVec();
    return QVector3D(v.x, v.y, v.z);
}

QVector3D Camera::forward() const
{
    const glm::vec3 v = forwardVec();
    return QVector3D(v.x, v.y, v.z);
}

void Camera::setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
{
    m_eye = eye;
    m_center = center;
    m_up = glm::normalize(up);
}

void Camera::orbit(float yawRadians, float pitchRadians)
{
    // Free trackball orbit of the eye around the center: yaw about the current
    // up, pitch about the camera's right axis. The up vector is carried through
    // the same rotation as the offset, so pitch can travel a full 360 degrees
    // over the poles — when the camera goes upside-down, up flips to point in
    // the -Y direction rather than being clamped.
    glm::vec3 offset = m_eye - m_center;
    if (glm::length(offset) <= 0.0f) {
        return;
    }
    const glm::vec3 up = glm::normalize(m_up);

    const glm::vec3 dir = glm::normalize(-offset);  // eye -> center
    glm::vec3 right = glm::cross(dir, up);
    const float rightLen = glm::length(right);
    // Looking straight along up: fall back to a stable axis so pitch still
    // works at the exact pole.
    right = rightLen > 0.0f ? right / rightLen : glm::vec3(1.0f, 0.0f, 0.0f);

    const glm::quat rotation =
        glm::angleAxis(-yawRadians, up) * glm::angleAxis(-pitchRadians, right);

    offset = rotation * offset;
    m_up = glm::normalize(rotation * up);
    m_eye = m_center + offset;
}

void Camera::pan(float dx, float dy)
{
    const glm::vec3 translation = -dx * rightVec() + dy * trueUpVec();
    m_eye += translation;
    m_center += translation;
}

void Camera::dolly(float amount)
{
    const float radius = distance();
    glm::vec3 newEye = m_eye + forwardVec() * amount;
    // Reject the step if it would cross (or land on top of) the center.
    if (glm::length(m_center - newEye) < m_minEyeDistance ||
        glm::dot(m_center - newEye, forwardVec()) <= 0.0f) 
    {
        newEye = m_center - forwardVec() * std::max(m_minEyeDistance, radius * 0.05f);
    }
    m_eye = newEye;
}

CameraConfig Camera::toConfig() const
{
    CameraConfig config;
    config.type = type();
    config.eye = m_eye;
    config.center = m_center;
    config.up = m_up;
    config.nearClip = m_near;
    config.farClip = m_far;
    writeProjection(config);
    return config;
}

void Camera::applyConfig(const CameraConfig& config)
{
    setLookAt(config.eye, config.center, config.up);
    m_near = config.nearClip;
    m_far = config.farClip;
    readProjection(config);
}

void Camera::commit(ANARIDevice device, ANARICamera camera) const
{
    if (!device || !camera) 
    {
        return;
    }

    const glm::vec3 dir = forwardVec();
    const std::array<float, 3> position{m_eye.x, m_eye.y, m_eye.z};
    const std::array<float, 3> direction{dir.x, dir.y, dir.z};
    const std::array<float, 3> up{m_up.x, m_up.y, m_up.z};

    anariSetParameter(device, camera, "position", ANARI_FLOAT32_VEC3, position.data());
    anariSetParameter(device, camera, "direction", ANARI_FLOAT32_VEC3, direction.data());
    anariSetParameter(device, camera, "up", ANARI_FLOAT32_VEC3, up.data());
    anariSetParameter(device, camera, "aspect", ANARI_FLOAT32, &m_aspect);
    anariSetParameter(device, camera, "near", ANARI_FLOAT32, &m_near);
    anariSetParameter(device, camera, "far", ANARI_FLOAT32, &m_far);

    applyProjection(device, camera);

    anariCommitParameters(device, camera);
}

} // namespace vitrine
