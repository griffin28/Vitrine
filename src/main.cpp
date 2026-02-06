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
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]).toLower();
        if (arg == "--light") {
            useDarkMode = false;
        } else if (arg == "--dark") {
            useDarkMode = true;
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
    const int deviceIndex = AppUtils::pickPhysicalDevice(triangleApp);
    triangleApp.setPhysicalDeviceIndex(deviceIndex);

    // Create and show the main application window
    myvulkan::VulkanMainWindow mainWindow(nullptr, &triangleApp);
    mainWindow.resize(WINDOW_WIDTH, WINDOW_HEIGHT);

    auto availableDevices = triangleApp.availablePhysicalDevices();
    mainWindow.appendLogMessage(QString("Selected Vulkan Physical Device: %1").arg(availableDevices[deviceIndex].deviceName));
    mainWindow.appendLogMessage(QString("Vulkan API Version: %1.%2.%3")
        .arg(VK_VERSION_MAJOR(availableDevices[deviceIndex].apiVersion))
        .arg(VK_VERSION_MINOR(availableDevices[deviceIndex].apiVersion))
        .arg(VK_VERSION_PATCH(availableDevices[deviceIndex].apiVersion)));
    mainWindow.appendLogMessage(QString("Driver Version: %1.%2.%3")
        .arg(VK_VERSION_MAJOR(availableDevices[deviceIndex].driverVersion))
        .arg(VK_VERSION_MINOR(availableDevices[deviceIndex].driverVersion))
        .arg(VK_VERSION_PATCH(availableDevices[deviceIndex].driverVersion)));
    mainWindow.appendLogMessage(QString("Vendor ID: %1").arg(availableDevices[deviceIndex].vendorID));
    mainWindow.appendLogMessage(QString("Device ID: %1").arg(availableDevices[deviceIndex].deviceID));

    mainWindow.show();
    return app.exec();
}

