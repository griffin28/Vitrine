#include "HelloTriangle.h"
#include "VulkanRenderer.h"

#include <QVulkanDeviceFunctions>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
QVulkanWindowRenderer* HelloTriangleApplication::createRenderer() {
    return new VulkanRenderer(this);
}
}