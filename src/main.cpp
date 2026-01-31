#include <QGuiApplication>
#include <vulkan/vulkan.h>
#include <QVulkanInstance>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "HelloTriangle.h"

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

int main(int argc, char *argv[]) 
{
    try {
        QGuiApplication app(argc, argv);
        QVulkanInstance vulkanInstance;
        // vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });
        if (!vulkanInstance.create()) {
            throw std::runtime_error("Failed to create Vulkan instance.");
        }

        myvulkan::HelloTriangleApplication htapp(WINDOW_WIDTH, WINDOW_HEIGHT);
        htapp.setVulkanInstance(&vulkanInstance);
        htapp.show();

        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

