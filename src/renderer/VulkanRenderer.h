#pragma once

#include <QVulkanWindow>

namespace myvulkan {

class VulkanRenderer final : public QVulkanWindowRenderer {
public:
    VulkanRenderer(QVulkanWindow* window)
        : m_window(window) {}
    ~VulkanRenderer() override = default;

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

private:
    QVulkanWindow* m_window = nullptr;
    QVulkanDeviceFunctions* m_devFuncs = nullptr;
};
}  // namespace myvulkan