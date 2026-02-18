#pragma once

#include <QVulkanWindowRenderer>

#include <QString>
#include <vulkan/vulkan.h>

namespace myvulkan {

class VulkanRenderer final : public QVulkanWindowRenderer 
{
public:
    /// @brief Constructor
    /// @param window the Vulkan window
    VulkanRenderer(QVulkanWindow* window)
        : m_window(window) {}

    /// @brief Destructor
    ~VulkanRenderer() override = default;

    /// @see QVulkanWindowRenderer::initResources
    void initResources() override;

    /// @see QVulkanWindowRenderer::initSwapChainResources
    void initSwapChainResources() override;

    /// @see QVulkanWindowRenderer::releaseSwapChainResources
    void releaseSwapChainResources() override;

    /// @see QVulkanWindowRenderer::releaseResources
    void releaseResources() override;

    /// @see QVulkanWindowRenderer::startNextFrame
    void startNextFrame() override;

private:
    QVulkanWindow* m_window = VK_NULL_HANDLE;
    QVulkanDeviceFunctions* m_devFuncs = VK_NULL_HANDLE;
    
    VkShaderModule createShaderModule(const QString& filePath);
};
}  // namespace myvulkan