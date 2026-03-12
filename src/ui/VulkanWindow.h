#pragma once

#include <QVulkanWindow>

namespace myvulkan 
{
class VulkanWindow : public QVulkanWindow 
{
public:
    /// @brief Constructor
    explicit VulkanWindow(QWindow* parent = nullptr) : QVulkanWindow(parent) {}

    /// @brief  Destructor
    ~VulkanWindow() = default;

    /// @see QVulkanWindow::createRenderer
    QVulkanWindowRenderer* createRenderer() override;
};
}  // namespace myvulkan