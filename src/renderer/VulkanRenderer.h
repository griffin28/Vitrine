#pragma once

#include <QVulkanWindowRenderer>

#include <QString>
#include <vulkan/vulkan.h>
// #include <vulkan/vulkan_raii.hpp>

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

    // Shader count constant
    static constexpr int ShaderCount = 2;

private:
    QVulkanWindow* m_window = VK_NULL_HANDLE;
    VkPipelineLayout* m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline* m_graphicsPipeline = VK_NULL_HANDLE;

    VkShaderModule createShaderModule(const QString& filePath);
    void recordCommandBuffer();
    void transitionImageLayout(VkCommandBuffer commandBuffer,
                               VkImageLayout oldLayout, 
                               VkImageLayout newLayout,
                               VkAccessFlags2 srcAccessMask,
                               VkAccessFlags2 dstAccessMask,
                               VkPipelineStageFlags2 srcStageMask,
                               VkPipelineStageFlags2 dstStageMask);
};
}  // namespace myvulkan