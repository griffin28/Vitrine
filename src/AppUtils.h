#pragma once

#include <QPalette>
#include <QApplication>
#include <QStyleFactory>
#include <QFile>

namespace myvulkan 
{
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