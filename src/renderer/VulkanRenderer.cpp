#include "VulkanRenderer.h"

namespace myvulkan {

void VulkanRenderer::initResources() {
    auto device = m_window->device();
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(device);
}

void VulkanRenderer::initSwapChainResources() {
    // Initialize swapchain-dependent resources here if needed.
}

void VulkanRenderer::releaseSwapChainResources() {
    // Release swapchain-dependent resources here if needed.
}

void VulkanRenderer::releaseResources() {
    // Release device-level resources here if needed.
}

void VulkanRenderer::startNextFrame() {
    // Signal that the frame is ready and schedule the next update.
    m_window->frameReady();
    m_window->requestUpdate();
}

}  // namespace myvulkan