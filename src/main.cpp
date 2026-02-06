#include <QApplication>
#include <vulkan/vulkan.h>
#include <QVulkanInstance>
#include <QStyleHints>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "HelloTriangle.h"
#include "VulkanMainWindow.h"
#include "AppUtils.h"

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

using AppUtils = myvulkan::AppUtils;

int main(int argc, char *argv[]) 
{
    // Allow style mode to be set via command line arguments
    bool useDarkMode = true;
    int gpuIndex = -1;
    
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]).toLower();
        if (arg == "--light") {
            useDarkMode = false;
        } else if (arg == "--dark") {
            useDarkMode = true;
        } else if (arg == "--gpu") {
            bool ok = false;
            QString indexStr = (i + 1 < argc) ? QString::fromLocal8Bit(argv[++i]) : QString();
            int index = indexStr.toInt(&ok);
            if (ok) {
                gpuIndex = index;
            } else {
                std::cerr << "Invalid GPU index provided after --gpu argument." << std::endl;
            }
        }
    }
    
    QApplication app(argc, argv);
    if (useDarkMode) {
        AppUtils::applyDarkMode(app);
    } else {
        AppUtils::applyLightMode(app);
    }
    QVulkanInstance vulkanInstance;

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

    if (!vulkanInstance.create()) 
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }

    // Create the Vulkan window
    myvulkan::HelloTriangleApplication triangleApp;
    triangleApp.setVulkanInstance(&vulkanInstance);

    auto availableDevices = triangleApp.availablePhysicalDevices();

    if(gpuIndex >= 0 && gpuIndex < availableDevices.size()) 
    {
        triangleApp.setPhysicalDeviceIndex(gpuIndex);
    } 
    else 
    {
        if(gpuIndex >= availableDevices.size()) 
        {
            std::cerr << "Warning: GPU index " << gpuIndex << " is out of range. Falling back to automatic device selection." << std::endl;
        }

        const int selectedGpuIndex = AppUtils::pickPhysicalDevice(availableDevices);
        triangleApp.setPhysicalDeviceIndex(selectedGpuIndex);
    }

    // Create and show the main application window
    myvulkan::VulkanMainWindow mainWindow(nullptr, &triangleApp);
    mainWindow.resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    mainWindow.show();

    return app.exec();
}

