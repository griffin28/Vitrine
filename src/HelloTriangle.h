#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULE)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <QVulkanWindow>

namespace myvulkan 
{
class HelloTriangleApplication : public QVulkanWindow 
{
public:
    /// @brief Constructor
    explicit HelloTriangleApplication(QWindow* parent = nullptr) : QVulkanWindow(parent) {}

    /// @brief  Destructor
    ~HelloTriangleApplication() = default;

    /// @see QVulkanWindow::createRenderer
    QVulkanWindowRenderer* createRenderer() override;
};
}  // namespace myvulkan