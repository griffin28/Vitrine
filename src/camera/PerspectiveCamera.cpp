#include "PerspectiveCamera.h"

namespace vitrine
{

void PerspectiveCamera::applyProjection(ANARIDevice device, ANARICamera camera) const
{
    anariSetParameter(device, camera, "fovy", ANARI_FLOAT32, &m_fovY);
}

} // namespace vitrine
