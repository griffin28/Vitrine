#include "HelloTriangle.h"
#include "VulkanRenderer.h"

namespace myvulkan {

HelloTriangleApplication::HelloTriangleApplication(const uint32_t width, const uint32_t height) 
{ 
    this->resize(width, height);
    this->setTitle("Hello Triangle Vulkan with Qt 6");
}

QVulkanWindowRenderer* HelloTriangleApplication::createRenderer() {
    return new VulkanRenderer(this);
}
}