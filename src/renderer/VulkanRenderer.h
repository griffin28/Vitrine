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

    /// @brief Get the size of the vertex buffer
    size_t getVertexBufferSize() const { return sizeof(Vertex) * m_vertices.size(); }

    /// @brief Get the size of the index buffer
    size_t getIndexBufferSize() const { return sizeof(uint32_t) * m_indices.size(); }

private:
    QVulkanWindow* m_window = nullptr;
    VkPipelineLayout* m_pipelineLayout = nullptr;
    VkPipeline* m_graphicsPipeline = nullptr;
    
    std::vector<Vertex> m_vertices;
    VkBuffer* m_vertexBuffer = nullptr;
    VkDeviceMemory* m_vertexBufferMemory = nullptr;

    std::vector<uint32_t> m_indices;
    VkBuffer* m_indexBuffer = nullptr;
    VkDeviceMemory* m_indexBufferMemory = nullptr;

    std::vector<VkBuffer *> m_uniformBuffers;
    std::vector<VkDeviceMemory *> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    VkDescriptorSetLayout* m_descriptorSetLayout = nullptr;
    VkDescriptorPool* m_descriptorPool = nullptr;
    std::vector<VkDescriptorSet> m_descriptorSets;

    uint32_t m_mipLevels = 0;
    VkImage* m_textureImage = nullptr;
    VkDeviceMemory* m_textureImageMemory = nullptr;
    VkImageView* m_textureImageView = nullptr;
    VkSampler* m_textureSampler = nullptr;

    VkShaderModule createShaderModule(const QString& filePath);
    VkResult createTextureImage(const QString& filePath, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
    VkResult createTextureSampler(VkSampler& textureSampler);
    VkResult createImageView(VkImage& image, 
                             VkFormat format, 
                             VkImageAspectFlags aspectFlags, 
                             VkImageView& imageView);
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

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, bool hostVisible = true);
    VkCommandBuffer beginSingleTimeCommands();
    VkResult endSingleTimeCommands(VkCommandBuffer& commandBuffer);
    bool hasStencilComponent(VkFormat format);
    VkResult generateMipmaps(VkImage& image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    void loadModel(const QString& path);
    void recordCommandBuffer();
    void updateUniformBuffer();
    VkResult copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkResult copyBufferToImage(const VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height);
    VkResult transitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkSampleCountFlagBits getMaxUsableSampleCount();
};
}  // namespace myvulkan