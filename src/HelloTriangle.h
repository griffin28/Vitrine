#pragma once

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