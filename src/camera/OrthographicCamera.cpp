#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace vitrine
{

glm::mat4 OrthographicCamera::projectionMatrix() const
{
    const float halfH = 0.5f * m_height;
    const float halfW = halfH * m_aspect;
    return glm::ortho(-halfW, halfW, -halfH, halfH, m_near, m_far);
}

void OrthographicCamera::applyProjection(ANARIDevice device, ANARICamera camera) const
{
    anariSetParameter(device, camera, "height", ANARI_FLOAT32, &m_height);
}

void OrthographicCamera::writeProjection(CameraConfig& config) const
{
    config.height = m_height;
}

void OrthographicCamera::readProjection(const CameraConfig& config)
{
    m_height = config.height;
}

} // namespace vitrine
