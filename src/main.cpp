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

constexpr const char* KSETTINGSORG = "KSG-Technology-Consulting";
constexpr const char* KSETTINGDOMAIN = "ksgtechconsulting.com";
constexpr const char* KSETTINGSAPP = "VulkanSandbox";

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

const QByteArrayList validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

using AppUtils = myvulkan::AppUtils;
using AppMainWindow = myvulkan::AppMainWindow;

// Debug output filter for Vulkan validation layer messages
bool debugOutputFilter(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
                       size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage)
{
    QString logMessage = QString::fromUtf8(pMessage);
    QString layerPrefix = QString::fromUtf8(pLayerPrefix);

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) 
    {
        // Append validation layer error messages to the log panel with ERROR level
        auto errorLogMessage = QString("%1::Validation Layer ERROR: %2").arg(layerPrefix).arg(logMessage);
        QMetaObject::invokeMethod(qApp->activeWindow(), [errorLogMessage]() {
            auto mainWindow = qobject_cast<AppMainWindow*>(qApp->activeWindow());
            if (mainWindow) {
                mainWindow->appendErrorLogMessage(errorLogMessage);
            }
        });
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) 
    {
        // Append validation layer warning messages to the log panel with WARNING level
        auto warnLogMessage = QString("%1::Validation Layer WARNING: %2").arg(layerPrefix).arg(logMessage);
        QMetaObject::invokeMethod(qApp->activeWindow(), [warnLogMessage]() {
            auto mainWindow = qobject_cast<AppMainWindow*>(qApp->activeWindow());
            if (mainWindow) {
                mainWindow->appendWarningLogMessage(warnLogMessage);
            }
        });
    } else {
        // Append validation layer info messages to the log panel with INFO level
        auto additionalInfo = QString("Additional Info: %1, %2, %3, %4")
                                .arg(objectType)
                                .arg(object)
                                .arg(location)
                                .arg(messageCode);
        auto infoLogMessage = QString("%1::Validation Layer INFO: %2 %3").arg(layerPrefix).arg(logMessage).arg(additionalInfo);
        QMetaObject::invokeMethod(qApp->activeWindow(), [infoLogMessage]() {
            auto mainWindow = qobject_cast<AppMainWindow*>(qApp->activeWindow());
            if (mainWindow) {
                mainWindow->appendInfoLogMessage(infoLogMessage);
            }
        });
    }

    return false;
}

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

    QApplication::setOrganizationName(KSETTINGSORG);
    QApplication::setOrganizationDomain(KSETTINGDOMAIN);
    QApplication::setApplicationName(KSETTINGSAPP);

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
                logMessages.append(QString("Validation layer '%1' not found. ").arg(layerName.constData()));
            }
        }

        if(validLayers.isEmpty()) 
        {
            logMessages.append(QString("No requested validation layers are available. Running without validation layers."));
        }
        else 
        {
            // Add debug utils extension if validation layers are enabled
            auto supportedExtensions = vulkanInstance.supportedExtensions();

            if (supportedExtensions.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) 
            {
                vulkanInstance.setExtensions(QByteArrayList() << VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                logMessages.append(QString("Enabling extension: %1. ").arg(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
                vulkanInstance.installDebugOutputFilter(debugOutputFilter);
            } else 
            {
                logMessages.append(QString("Extension %1 not supported. Running without it. ").arg(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
            }

            logMessages.append(QString("Enabling validation layers: %1").arg(validLayers.join(", ")));
            vulkanInstance.setLayers(validLayers);
        }
    }

    // Set the Vulkan API version
    auto supportedApiVersion = vulkanInstance.supportedApiVersion();

    if (supportedApiVersion >= QVersionNumber(1,4)) 
    {
        vulkanInstance.setApiVersion(QVersionNumber(1, 4));
        logMessages.append(QString(". Requesting Vulkan API version 1.4. Supported version: %1.%2.%3")
                          .arg(supportedApiVersion.majorVersion())
                          .arg(supportedApiVersion.minorVersion())
                          .arg(supportedApiVersion.microVersion()));
    } 
    else if (supportedApiVersion >= QVersionNumber(1, 3)) 
    {
        vulkanInstance.setApiVersion(QVersionNumber(1, 3));
        logMessages.append(QString("; Requesting Vulkan API version 1.3. Supported version: %1.%2.%3")
                          .arg(supportedApiVersion.majorVersion())
                          .arg(supportedApiVersion.minorVersion())
                          .arg(supportedApiVersion.microVersion()));
    } 
    else if(supportedApiVersion >= QVersionNumber(1, 2)) 
    {
        vulkanInstance.setApiVersion(QVersionNumber(1, 2));
        logMessages.append(QString("; Requesting Vulkan API version 1.2. Supported version: %1.%2.%3")
                          .arg(supportedApiVersion.majorVersion())
                          .arg(supportedApiVersion.minorVersion())
                          .arg(supportedApiVersion.microVersion()));
    } 
    else 
    {
        vulkanInstance.setApiVersion(supportedApiVersion);
        logMessages.append(QString("; Vulkan API version 1.2 or higher is not supported. Supported version: %1.%2.%3")
                          .arg(supportedApiVersion.majorVersion())
                          .arg(supportedApiVersion.minorVersion())
                          .arg(supportedApiVersion.microVersion()));
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

