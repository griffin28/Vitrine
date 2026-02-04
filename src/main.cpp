#include <QApplication>
#include <vulkan/vulkan.h>
#include <QVulkanInstance>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "HelloTriangle.h"
#include "VulkanMainWindow.h"

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

const QByteArrayList validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

int pickPhysicalDevice(QVulkanWindow &window) {
    auto availableDevices = window.availablePhysicalDevices();

    if (availableDevices.isEmpty()) {
        throw std::runtime_error("No Vulkan-compatible physical devices found.");
    }

    for(int i = 0; i < availableDevices.size(); ++i) 
    {
        bool supportsVulkan1_4 = availableDevices[i].apiVersion >= VK_API_VERSION_1_4;
        
        if (!supportsVulkan1_4) 
        {
            continue;
        }

        // pick an NVIDIA GPU if available
        if (availableDevices[i].vendorID == 0x10DE) // NVIDIA's vendor ID
        {
            std::cout << "Selected Device " << i << ": " << availableDevices[i].deviceName << std::endl;
            std::cout << "  API Version: " 
                      << VK_VERSION_MAJOR(availableDevices[i].apiVersion) << "."
                      << VK_VERSION_MINOR(availableDevices[i].apiVersion) << "."
                      << VK_VERSION_PATCH(availableDevices[i].apiVersion) << std::endl;
            std::cout << "  Driver Version: " 
                      << VK_VERSION_MAJOR(availableDevices[i].driverVersion) << "."
                      << VK_VERSION_MINOR(availableDevices[i].driverVersion) << "."
                      << VK_VERSION_PATCH(availableDevices[i].driverVersion) << std::endl;
            std::cout << "  Vendor ID: " << availableDevices[i].vendorID << std::endl;
            std::cout << "  Device ID: " << availableDevices[i].deviceID << std::endl;
            
            return i;
        }
    }

    // If no NVIDIA GPU found, just return the first device that supports Vulkan 1.4
    for (int i = 0; i < availableDevices.size(); ++i) {
        if (availableDevices[i].apiVersion >= VK_API_VERSION_1_4) {
            return i;
        }
    }

    // If no device supports Vulkan 1.4, throw an exception
    throw std::runtime_error("No suitable Vulkan physical device found that supports Vulkan 1.4.");
}

int main(int argc, char *argv[]) 
{
    QApplication app(argc, argv);
    QVulkanInstance vulkanInstance;
    
    if (!vulkanInstance.create()) 
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }

    // Enable validation layers if in debug mode
    if (enableValidationLayers) {
        auto availableLayers = vulkanInstance.supportedLayers();

        for (const QByteArray& layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (layerName == layerProperties.name) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                throw std::runtime_error("Validation layer requested but not available: " + std::string(layerName.constData()));
            }
        }

        vulkanInstance.setLayers(validationLayers);
    }

    // Create the Vulkan window
    myvulkan::HelloTriangleApplication triangleApp;

    triangleApp.setVulkanInstance(&vulkanInstance);
    const int deviceIndex = pickPhysicalDevice(triangleApp);
    triangleApp.setPhysicalDeviceIndex(deviceIndex);

    // Create and show the main application window
    myvulkan::VulkanMainWindow mainWindow(nullptr, &triangleApp);
    mainWindow.resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    mainWindow.show();

    return app.exec();
}

