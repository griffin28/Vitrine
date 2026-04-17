#pragma once

#include <QPalette>
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QVulkanFunctions>

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace myvulkan 
{

constexpr uint32_t NVIDIA_VENDOR_ID = 0x10DE;

//----------------------------------------------------------------------------------
struct UniformBufferObject 
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;

    /// @brief Get a descriptor set layout binding for this uniform buffer object.
    /// @param binding the binding index to use for this uniform buffer object
    /// @return a VkDescriptorSetLayoutBinding describing how this uniform buffer object should be bound to a descriptor set
    static VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding(uint32_t binding) 
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = binding;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        return uboLayoutBinding;
    }
};

//----------------------------------------------------------------------------------
/// @brief Vertex structure representing a single vertex with position and color attributes.
struct Vertex 
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    /// @brief Equality operator for Vertex structure.
    /// @param other The other vertex to compare with.
    /// @return true if the vertices are equal, false otherwise.
    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }

    /// @brief Get the vertex input binding description for this vertex structure.
    /// @details This describes at which rate to load data from memory throughout the vertices. 
    ///          It specifies the number of bytes between data entries and whether to move to the next
    ///          data entry after each vertex or after each instance.
    /// @return a VkVertexInputBindingDescription describing how vertex data is laid out in memory
    static VkVertexInputBindingDescription getBindingDescription() 
    {
        VkVertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // VkVertexInputBindingDescription bindingDescription{
        //     .binding = 0,
        //     .stride = sizeof(Vertex),
        //     .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        // };
        
        return bindingDescription;
    }

    /// @brief Get the vertex input attribute descriptions for this vertex structure.
    /// @details This describes how to extract vertex attributes from the vertex data.
    ///          It specifies the location, format, and offset of each attribute within the vertex data.
    ///          The location corresponds to the location specified in the vertex shader for each attribute.
    /// @return an array of VkVertexInputAttributeDescription describing how to extract vertex attributes
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() 
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        // Position attribute
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // Color attribute
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        // Texture coordinate attribute
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

//----------------------------------------------------------------------------------
/// @brief Utility class for application-wide helper functions, such as picking a Vulkan physical device and applying stylesheets.
class AppUtils 
{
public:
    static inline int pickPhysicalDevice(QVulkanInstance* const vulkanInstance) 
    {
        auto funcs = vulkanInstance->functions();
        if (!funcs)        {
            throw std::runtime_error("Failed to load Vulkan device functions.");
        }
        
        uint32_t deviceCount = 0;
        VkResult result = funcs->vkEnumeratePhysicalDevices(vulkanInstance->vkInstance(), &deviceCount, nullptr);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to enumerate physical devices: " + std::to_string(result));
        }

        if (deviceCount == 0) {
            throw std::runtime_error("No Vulkan-compatible physical devices found.");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        result = funcs->vkEnumeratePhysicalDevices(vulkanInstance->vkInstance(), &deviceCount, devices.data());
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to enumerate physical devices: " + std::to_string(result));
        }
        std::vector<uint32_t> suitableDeviceIndices;

        for(uint32_t i = 0; i < deviceCount; ++i) 
        {
            // Get Physical Device Properties
            VkPhysicalDeviceProperties deviceProperties;
            funcs->vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

            // vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
            const bool supportsVulkan1_3 = deviceProperties.apiVersion >= VK_API_VERSION_1_3;
            if(!supportsVulkan1_3) 
            {
                qDebug() << "Device " << i << " does not support Vulkan 1.3, skipping.";
                continue;
            }

            // Get Queue Family Properties
            uint32_t queueFamilyCount = 0;
            funcs->vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);

            if (queueFamilyCount == 0) {
                qDebug() << "Device " << i << " has no queue families, skipping.";
                continue;
            }  
            
            std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
            funcs->vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilyProperties.data());

            bool hasGraphicsAndComputeQueue = false;

            for(const auto& qfp : queueFamilyProperties) 
            {
                if((qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (qfp.queueFlags & VK_QUEUE_COMPUTE_BIT)) 
                {
                    hasGraphicsAndComputeQueue = true;
                    break;
                }
            }

            if (!hasGraphicsAndComputeQueue) 
            {
                qDebug() << "Device " << i << " has no graphics and compute queue, skipping.";
                continue;
            }

            // prefer an NVIDIA GPU if available
            if (deviceProperties.vendorID == NVIDIA_VENDOR_ID)
            {   
                return i;
            }

            suitableDeviceIndices.push_back(i);
        }

        if (!suitableDeviceIndices.empty()) 
        {
            return suitableDeviceIndices[0];
        }

        throw std::runtime_error("No suitable Vulkan physical device found that supports Vulkan 1.3.");
    }

    /// @brief Apply a dark mode stylesheet to the given QApplication.
    /// @param app the QApplication instance to apply the dark mode to
    static inline void applyDarkMode(QApplication &app) 
    {
        QFile styleFile(":/qdarkstyle/dark/darkstyle.qss");
        
        if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }

    /// @brief Apply a light mode stylesheet to the given QApplication.
    /// @param app the QApplication instance to apply the light mode to
    static inline void applyLightMode(QApplication &app) 
    {
        QFile styleFile(":/qdarkstyle/light/lightstyle.qss");
        
        if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }
};
} // namespace myvulkan

// Custom specialization of std::hash for myvulkan::Vertex
// Allows it to be used as a key in unordered_map
// Gets injected into the namespace std
template <>
struct std::hash<myvulkan::Vertex>
{
    size_t operator()(const myvulkan::Vertex& vertex) const noexcept
    {
        size_t h1 = std::hash<float>()(vertex.pos.x) ^ (std::hash<float>()(vertex.pos.y) << 1) ^ (std::hash<float>()(vertex.pos.z) << 2);
        size_t h2 = std::hash<float>()(vertex.color.x) ^ (std::hash<float>()(vertex.color.y) << 1) ^ (std::hash<float>()(vertex.color.z) << 2);
        size_t h3 = std::hash<float>()(vertex.texCoord.x) ^ (std::hash<float>()(vertex.texCoord.y) << 1);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};