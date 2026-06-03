#include "CameraConfig.h"
#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"

namespace vitrine
{

std::unique_ptr<Camera> makeCamera(CameraType type)
{
    switch (type) 
    {
        case CameraType::Orthographic:
            return std::make_unique<OrthographicCamera>();
        case CameraType::Perspective:
        default:
            return std::make_unique<PerspectiveCamera>();
    }
}

} // namespace vitrine
