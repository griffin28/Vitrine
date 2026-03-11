#pragma once

#include <QVulkanWindowRenderer>
#include <QString>

#include "AppUtils.h"

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

    size_t getVertexBufferSize() const { return sizeof(Vertex) * this->getVertices().size(); }
    size_t getIndexBufferSize() const { return sizeof(uint16_t) * this->getIndices().size(); }

    std::vector<Vertex> getVertices() const
    {
        return {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left vertex (red)
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right vertex (green)
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},  // Top-right vertex (blue)
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}}   // Top-left vertex (yellow)
        };
    }

    std::vector<uint16_t> getIndices() const
    {
        return {
            0, 1, 2, // First triangle (bottom-right)
            2, 3, 0  // Second triangle (top-left)
        };
    }

private:
    QVulkanWindow* m_window = nullptr;
    VkPipelineLayout* m_pipelineLayout = nullptr;
    VkPipeline* m_graphicsPipeline = nullptr;

    VkBuffer* m_vertexBuffer = nullptr;
    VkDeviceMemory* m_vertexBufferMemory = nullptr;

    VkBuffer* m_indexBuffer = nullptr;
    VkDeviceMemory* m_indexBufferMemory = nullptr;

    VkShaderModule createShaderModule(const QString& filePath);
    void createVertexBuffer();
    void createIndexBuffer();
    VkResult createBuffer(VkDeviceSize size, 
                          VkBufferUsageFlags usage, 
                          VkMemoryPropertyFlags properties, 
                          uint32_t memoryTypeIndex,
                          VkBuffer& buffer, 
                          VkDeviceMemory& bufferMemory);
    VkResult copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, bool hostVisible = true);
    void recordCommandBuffer(const int frameIndex);
};
}  // namespace myvulkan