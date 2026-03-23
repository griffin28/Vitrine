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
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {2.f, 0.0f}}, // Bottom-left vertex (red)
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},  // Bottom-right vertex (green)
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 2.f}},   // Top-right vertex (blue)
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}, {2.f, 2.f}}   // Top-left vertex (yellow)
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

    std::vector<VkBuffer *> m_uniformBuffers;
    std::vector<VkDeviceMemory *> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    VkDescriptorSetLayout* m_descriptorSetLayout = nullptr;
    VkDescriptorPool* m_descriptorPool = nullptr;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkImage* m_textureImage = nullptr;
    VkDeviceMemory* m_textureImageMemory = nullptr;
    VkImageView* m_textureImageView = nullptr;
    VkSampler* m_textureSampler = nullptr;

    VkShaderModule createShaderModule(const QString& filePath);
    VkResult createTextureImage(const QString& filePath, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
    VkResult createTextureImageView(VkImage& textureImage, VkImageView& textureImageView);
    VkResult createTextureSampler(VkSampler& textureSampler);
    VkResult createImage(uint32_t width, uint32_t height, 
                    VkFormat format, 
                    VkImageTiling tiling, 
                    VkImageUsageFlags usage, 
                    VkMemoryPropertyFlags properties, 
                    VkImage& image, 
                    VkDeviceMemory& imageMemory);
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    VkResult createDescriptorPool();
    VkResult createDescriptorSets();
    VkResult createBuffer(VkDeviceSize size, 
                          VkBufferUsageFlags usage, 
                          VkMemoryPropertyFlags properties, 
                          uint32_t memoryTypeIndex,
                          VkBuffer& buffer, 
                          VkDeviceMemory& bufferMemory);

    VkCommandBuffer beginSingleTimeCommands();
    VkResult endSingleTimeCommands(VkCommandBuffer& commandBuffer);

    VkResult copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkResult copyBufferToImage(const VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, bool hostVisible = true);
    void recordCommandBuffer();
    void updateUniformBuffer();
    VkResult transitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);
};
}  // namespace myvulkan