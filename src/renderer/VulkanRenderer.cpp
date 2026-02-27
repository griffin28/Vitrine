#include "VulkanRenderer.h"

#include <QVulkanDeviceFunctions>
#include <vulkan/vulkan.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStringList>

namespace myvulkan {

//----------------------------------------------------------------------------------
void VulkanRenderer::initResources() 
{
    auto instance = m_window->vulkanInstance();
    auto device = m_window->device();
    m_devFuncs = instance->deviceFunctions(device);

    // Create shader module
    const QString appDir = qApp->applicationDirPath(); 
    const QString fragPath = QDir(appDir).filePath("shaders/frag.spv");
    const QString vertPath = QDir(appDir).filePath("shaders/vert.spv");

    auto vertShaderModule = createShaderModule(vertPath);
    auto fragShaderModule = createShaderModule(fragPath);

    // Shader stage creation
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };
}

//----------------------------------------------------------------------------------
void VulkanRenderer::initSwapChainResources() {
    // Initialize swapchain-dependent resources here if needed.
    // m_window->physicalDevice()->getSurfaceCapabilitiesKHR(m_window->surface(), &m_surfaceCapabilities);
    // auto surfaceFormats = m_window->physicalDevice()->getSurfaceFormatsKHR(QVulkanInstance::surfaceForWindow(m_window));
}

//----------------------------------------------------------------------------------
void VulkanRenderer::releaseSwapChainResources() {
    // Release swapchain-dependent resources here if needed.
}

//----------------------------------------------------------------------------------
void VulkanRenderer::releaseResources() 
{
    // Release device-level resources here if needed.
    // if (m_shaderModule != VK_NULL_HANDLE) {
    //     m_devFuncs->vkDestroyShaderModule(m_window->device(), m_shaderModule, nullptr);
    //     m_shaderModule = VK_NULL_HANDLE;
    // }
}

//----------------------------------------------------------------------------------
void VulkanRenderer::startNextFrame() 
{
    // Signal that the frame is ready and schedule the next update.
    m_window->frameReady();
    m_window->requestUpdate();
}

//----------------------------------------------------------------------------------
VkShaderModule VulkanRenderer::createShaderModule(const QString& filePath) 
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) 
    {
        qWarning() << "Failed to open shader file:" << filePath;
        return VK_NULL_HANDLE;
    }

    const QByteArray shaderCode = file.readAll();
    if (shaderCode.isEmpty()) 
    {
        qWarning() << "Shader file is empty:" << filePath;
        return VK_NULL_HANDLE;
    }

    auto bytesRead = shaderCode.size();
    auto kilobytesRead = bytesRead / 1024.0;
    qInfo() << "Read" << bytesRead << "bytes (" << kilobytesRead << " KB) from shader file:" << filePath;

    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<size_t>(shaderCode.size()),
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.constData())
    };

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    const VkResult result = m_devFuncs->vkCreateShaderModule(
        m_window->device(),
        &createInfo,
        nullptr,
        &shaderModule);

    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create shader module from" << filePath << "VkResult:" << result;
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

}  // namespace myvulkan