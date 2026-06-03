#include "PerspectiveCamera.h"

namespace vitrine
{

void PerspectiveCamera::applyProjection(ANARIDevice device, ANARICamera camera) const
{
    anariSetParameter(device, camera, "fovy", ANARI_FLOAT32, &m_fovY);
}

void PerspectiveCamera::writeProjection(CameraConfig& config) const
{
    config.fovYDegrees = glm::degrees(m_fovY);
}

void PerspectiveCamera::readProjection(const CameraConfig& config)
{
    m_fovY = glm::radians(config.fovYDegrees);
}

} // namespace vitrine
