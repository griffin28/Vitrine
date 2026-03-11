#pragma once

#include <QPalette>
#include <QApplication>
#include <QStyleFactory>
#include <QFile>

#include <glm/glm.hpp>
#include <array>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
/// @brief Vertex structure representing a single vertex with position and color attributes.
struct Vertex 
{
    glm::vec2 pos;
    glm::vec3 color;

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
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() 
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        // Position attribute
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // Color attribute
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

//----------------------------------------------------------------------------------
/// @brief Utility class for application-wide helper functions, such as picking a Vulkan physical device and applying stylesheets.
class AppUtils 
{
public:
    /// @brief Pick a suitable Vulkan physical device, preferring NVIDIA GPUs if available.
    /// @param availableDevices a list of available Vulkan physical devices and their properties
    /// @return device index of the selected physical device
    /// @throws std::runtime_error if no suitable device is found
    static inline int pickPhysicalDevice(const QList<VkPhysicalDeviceProperties> &availableDevices) 
    {
        if (availableDevices.isEmpty()) {
            throw std::runtime_error("No Vulkan-compatible physical devices found.");
        }

        for(int i = 0; i < availableDevices.size(); ++i) 
        {
            bool supportsVulkan1_3 = availableDevices[i].apiVersion >= VK_API_VERSION_1_3;

            // pick an NVIDIA GPU if available
            if (supportsVulkan1_3 && availableDevices[i].vendorID == 0x10DE) // NVIDIA's vendor ID
            {   
                return i;
            }
        }

        // If no NVIDIA GPU found, just return the first device that supports Vulkan 1.3
        for (int i = 0; i < availableDevices.size(); ++i) {
            if (availableDevices[i].apiVersion >= VK_API_VERSION_1_3) {
                return i;
            }
        }

        // If no device supports Vulkan 1.3, throw an exception
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
}