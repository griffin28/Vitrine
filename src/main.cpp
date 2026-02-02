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

int main(int argc, char *argv[]) 
{
    QApplication app(argc, argv);
    QVulkanInstance vulkanInstance;
    // vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    if (!vulkanInstance.create()) 
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }

    // Create the Vulkan window
    myvulkan::HelloTriangleApplication triangleApp;
    triangleApp.setVulkanInstance(&vulkanInstance);

    // Create and show the main application window
    myvulkan::VulkanMainWindow mainWindow(nullptr, &triangleApp);
    mainWindow.resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    mainWindow.show();

    return app.exec();
}

