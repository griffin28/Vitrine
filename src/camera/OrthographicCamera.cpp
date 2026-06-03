#include "OrthographicCamera.h"

namespace vitrine
{

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
