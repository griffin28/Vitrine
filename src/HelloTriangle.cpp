#include "HelloTriangle.h"
#include "VulkanRenderer.h"

namespace myvulkan 
{
//----------------------------------------------------------------------------------
QVulkanWindowRenderer* HelloTriangleApplication::createRenderer() {
    return new VulkanRenderer(this);
}
}