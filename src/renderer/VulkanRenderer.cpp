#include "VulkanRenderer.h"

#include <QVulkanDeviceFunctions>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStringList>
#include <iostream>

namespace myvulkan {

//----------------------------------------------------------------------------------
void VulkanRenderer::initResources() 
{
    auto instance = m_window->vulkanInstance();
    auto device = m_window->device();
    auto devFuncs = instance->deviceFunctions(device);

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

    // Dynamic states (specify at draw time)
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr, // Dynamic state
        .scissorCount = 1,
        .pScissors = nullptr // Dynamic state
    };

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f,
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // Depth and Stencil Testing
    // VkPipelineDepthStencilStateCreateInfo depthStencil{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    //     .depthTestEnable = VK_TRUE,
    //     .depthWriteEnable = VK_TRUE,
    //     .depthCompareOp = VK_COMPARE_OP_LESS,
    //     .depthBoundsTestEnable = VK_FALSE,
    //     .stencilTestEnable = VK_FALSE,
    //     .front = {},
    //     .back = {},
    //     .minDepthBounds = 0.0f,
    //     .maxDepthBounds = 1.0f
    // };

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    m_pipelineLayout = new VkPipelineLayout;
    VkResult result = devFuncs->vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, m_pipelineLayout);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create pipeline layout, VkResult:" << result;
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Pipeline Rendering
    VkFormat colorFormat = m_window->colorFormat();
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = m_window->depthStencilFormat(),
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr, // Optional
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicStateCreateInfo, // Optional
        .layout = *m_pipelineLayout,
        .renderPass = VK_NULL_HANDLE, // Using dynamic rendering
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1 // Optional
    };

    m_graphicsPipeline = new VkPipeline;
    result = devFuncs->vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, m_graphicsPipeline);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create graphics pipeline, VkResult:" << result;
        throw std::runtime_error("Failed to create graphics pipeline");
    }
}

//----------------------------------------------------------------------------------
void VulkanRenderer::initSwapChainResources() 
{
    // Color Format
    // std::cout << "Color Format: " << m_window->colorFormat() << std::endl;

    // Depth Format
    // std::cout << "Depth Format: " << m_window->depthStencilFormat() << std::endl;

    // Image Size
    // std::cout << "Image Size: " << m_window->swapChainImageSize().width() << "x" << m_window->swapChainImageSize().height() << std::endl;

    // Image Count
    // std::cout << "Image Count: " << m_window->swapChainImageCount() << std::endl;

    // Initialize swapchain-dependent resources here if needed.
    // auto instance = m_window->vulkanInstance();
    // auto physicalDevice = m_window->physicalDevice();
    // auto surface = QVulkanInstance::surfaceForWindow(m_window);
    
    // // Basic Surface Capabilities
    // auto vkGetPhysicalDeviceSurfaceCapabilitiesKHR = 
    //     reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
    //         instance->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    
    // if (!vkGetPhysicalDeviceSurfaceCapabilitiesKHR) {
    //     qWarning() << "Failed to load vkGetPhysicalDeviceSurfaceCapabilitiesKHR";
    //     return;
    // }

    // VkSurfaceCapabilitiesKHR surfaceCapabilities;
    // VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    
    // if (result != VK_SUCCESS) {
    //     qWarning() << "Failed to get surface capabilities, VkResult:" << result;
    //     return;
    // }

    // Print surface capabilities
    // std::cout << "Surface Capabilities:" << std::endl;
    // std::cout << "  Min Image Count: " << surfaceCapabilities.minImageCount << std::endl;
    // std::cout << "  Max Image Count: " << surfaceCapabilities.maxImageCount << std::endl;
    // std::cout << "  Current Extent: " << surfaceCapabilities.currentExtent.width << "x" << surfaceCapabilities.currentExtent.height << std::endl;
    // std::cout << "  Min Image Extent: " << surfaceCapabilities.minImageExtent.width << "x" << surfaceCapabilities.minImageExtent.height << std::endl;
    // std::cout << "  Max Image Extent: " << surfaceCapabilities.maxImageExtent.width << "x" << surfaceCapabilities.maxImageExtent.height << std::endl;
    // std::cout << "  Max Image Array Layers: " << surfaceCapabilities.maxImageArrayLayers << std::endl;
    // std::cout << "  Supported Transforms: " << surfaceCapabilities.supportedTransforms << std::endl;
    // std::cout << "  Current Transform: " << surfaceCapabilities.currentTransform << std::endl;
    // std::cout << "  Supported Composite Alpha: " << surfaceCapabilities.supportedCompositeAlpha << std::endl;
    // std::cout << "  Supported Usage Flags: " << surfaceCapabilities.supportedUsageFlags << std::endl;

    // Surface Formats (pixel format, color space)
    // auto vkGetPhysicalDeviceSurfaceFormatsKHR = 
    //     reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
    //         instance->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));

    // if (!vkGetPhysicalDeviceSurfaceFormatsKHR) {
    //     qWarning() << "Failed to load vkGetPhysicalDeviceSurfaceFormatsKHR";
    //     return;
    // }

    // // Get the number of supported surface formats
    // uint32_t formatCount = 0;
    // result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    // if (result != VK_SUCCESS || formatCount == 0) {
    //     qWarning() << "Failed to get surface format count, VkResult:" << result;
    //     return;
    // }

    // std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    // result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());
    // if (result != VK_SUCCESS) {
    //     qWarning() << "Failed to get surface formats, VkResult:" << result;
    //     return;
    // }

    // // Print supported surface formats
    // // std::cout << "Supported Surface Formats:" << std::endl;
    // // for (const auto& format : surfaceFormats) {
    // //     std::cout << "  Format: " << format.format << ", Color Space: " << format.colorSpace << std::endl;
    // // }

    // // Present Modes
    // auto vkGetPhysicalDeviceSurfacePresentModesKHR = 
    //     reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
    //         instance->getInstanceProcAddr("vkGetPhysicalDeviceSurfacePresentModesKHR"));

    // if (!vkGetPhysicalDeviceSurfacePresentModesKHR) {
    //     qWarning() << "Failed to load vkGetPhysicalDeviceSurfacePresentModesKHR";
    //     return;
    // }

    // uint32_t presentModeCount = 0;
    // result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    // if (result != VK_SUCCESS || presentModeCount == 0) {
    //     qWarning() << "Failed to get present mode count, VkResult:" << result;
    //     return;
    // }

    // std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    // result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
    // if (result != VK_SUCCESS) {
    //     qWarning() << "Failed to get present modes, VkResult:" << result;
    //     return;
    // }

    // // Print supported present modes
    // std::cout << "Supported Present Modes:" << std::endl;
    // for (const auto& mode : presentModes) {
    //     std::cout << "  Present Mode: " << mode << std::endl;
    // }

    // Update swap chain info
    // VkSwapchainCreateInfoKHR swapChainInfo{
    //     .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    //     .surface = surface,
    //     .minImageCount = 2,
    //     .clipped = VK_TRUE
    // };

    // VkSwapchainKHR swapChain;
    // auto vkCreateSwapchainKHR = 
    //     reinterpret_cast<PFN_vkCreateSwapchainKHR>(
    //         m_window->vulkanInstance()->getInstanceProcAddr("vkCreateSwapchainKHR"));
    
    // if (!vkCreateSwapchainKHR) {
    //     qWarning() << "Failed to load vkCreateSwapchainKHR";
    //     return;
    // }

    // result = vkCreateSwapchainKHR(m_window->device(), &swapChainInfo, nullptr, &swapChain);

    // if (result != VK_SUCCESS) {
    //     qWarning() << "Failed to create swap chain, VkResult:" << result;
    //     return;
    // }
}
//----------------------------------------------------------------------------------
void VulkanRenderer::releaseSwapChainResources() {
    // Release swapchain-dependent resources here if needed.
}

//----------------------------------------------------------------------------------
void VulkanRenderer::releaseResources() 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    // Pipeline Layout
    if(m_pipelineLayout) 
    {
        devFuncs->vkDestroyPipelineLayout(m_window->device(), *m_pipelineLayout, nullptr);
        delete m_pipelineLayout;
        m_pipelineLayout = nullptr;
    }
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

    VkShaderModule shaderModule;
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    const VkResult result = devFuncs->vkCreateShaderModule(
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