#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULE)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <QVulkanWindow>

namespace myvulkan {

class HelloTriangleApplication : public QVulkanWindow {
public:
    HelloTriangleApplication() { HelloTriangleApplication(800, 600); }

    /// @brief  Constructor with custom width and height
    /// @param width width of the window
    /// @param height height of the window
    HelloTriangleApplication(const uint32_t width, const uint32_t height);

    /// @brief  Destructor
    ~HelloTriangleApplication() override = default;

    /// @see QVulkanWindow::createRenderer
    QVulkanWindowRenderer* createRenderer() override;
};
}  // namespace myvulkan