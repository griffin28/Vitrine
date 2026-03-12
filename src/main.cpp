#include <QApplication>
#include <QStyleHints>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "AppMainWindow.h"
#include "AppUtils.h"

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

const QByteArrayList validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

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
    QString logMessages;

    // Enable validation layers if in debug mode
    if (enableValidationLayers) {
        auto availableLayers = vulkanInstance.supportedLayers();
        QByteArrayList validLayers;

        for (const QByteArray& layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (layerName == layerProperties.name) {
                    layerFound = true;
                    validLayers.append(layerName);
                    break;
                }
            }

            if (!layerFound) {
                logMessages.append(QString("Validation layer '%1' not found.").arg(layerName.constData()));
            }
        }

        if(validLayers.isEmpty()) 
        {
            logMessages.append(QString("No requested validation layers are available. Running without validation layers."));
        }
        else 
        {
            logMessages.append(QString("Enabling validation layers: %1").arg(validLayers.join(", ")));
            vulkanInstance.setLayers(validLayers);
        }
    }

    if (!vulkanInstance.create()) 
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }

    // Create and show the main application window
    myvulkan::AppMainWindow mainWindow(&vulkanInstance, 
                                        logMessages, 
                                        gpuIndex);
    mainWindow.resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    mainWindow.show();

    return app.exec();
}

