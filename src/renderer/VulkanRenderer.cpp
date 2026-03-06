#include "VulkanRenderer.h"

#include <QVulkanDeviceFunctions>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStringList>
#include <cstring>
#include <iostream>
#include <vector>

namespace myvulkan {

//----------------------------------------------------------------------------------
void VulkanRenderer::initResources() 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    // Create shader module
    const QString appDir = qApp->applicationDirPath(); 
    const QString fragPath = QDir(appDir).filePath("shaders/frag.spv");
    const QString vertPath = QDir(appDir).filePath("shaders/vert.spv");

    auto vertShaderModule = this->createShaderModule(vertPath);
    auto fragShaderModule = this->createShaderModule(fragPath);

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
    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };

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
        .pushConstantRangeCount = 0
    };

    m_pipelineLayout = new VkPipelineLayout;
    VkResult result = devFuncs->vkCreatePipelineLayout(m_window->device(), &pipelineLayoutInfo, nullptr, m_pipelineLayout);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create pipeline layout, VkResult:" << result;
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicStateCreateInfo, // Optional
        .layout = *m_pipelineLayout,
        .renderPass = m_window->defaultRenderPass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1 // Optional
    };

    m_graphicsPipeline = new VkPipeline;
    result = devFuncs->vkCreateGraphicsPipelines(m_window->device(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, m_graphicsPipeline);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create graphics pipeline, VkResult:" << result;
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // Delete shader modules
    devFuncs->vkDestroyShaderModule(m_window->device(), vertShaderModule, nullptr);
    devFuncs->vkDestroyShaderModule(m_window->device(), fragShaderModule, nullptr);
}

//----------------------------------------------------------------------------------
void VulkanRenderer::releaseResources() 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    // Graphics Pipeline
    if(m_graphicsPipeline) 
    {
        devFuncs->vkDestroyPipeline(m_window->device(), *m_graphicsPipeline, nullptr);
        delete m_graphicsPipeline;
        m_graphicsPipeline = nullptr;
    }

    // Pipeline Layout
    if(m_pipelineLayout) 
    {
        devFuncs->vkDestroyPipelineLayout(m_window->device(), *m_pipelineLayout, nullptr);
        delete m_pipelineLayout;
        m_pipelineLayout = nullptr;
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
}

//----------------------------------------------------------------------------------
void VulkanRenderer::releaseSwapChainResources() 
{
    // Release swapchain-dependent resources here if needed.
}

//----------------------------------------------------------------------------------
// Wait for the previous frame to finish
// Acquire an image from the swap chain
// Record a command buffer which draws the scene onto that image
// Submit the recorded command buffer
// Present the swap chain image
//
void VulkanRenderer::startNextFrame() 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    // QVulkanWindow performs acquire/submit/present internally after frameReady().
    // Waiting for the graphics queue here avoids reusing present wait semaphores
    // while they may still be pending in the presentation engine.
    if (devFuncs->vkQueueWaitIdle(m_window->graphicsQueue()) != VK_SUCCESS)
    {
        qWarning() << "Failed to wait for graphics queue idle";
        return;
    }

    this->recordCommandBuffer();

    // Signal that the frame is ready and schedule the next update.
    m_window->frameReady();
    m_window->requestUpdate();
}

//----------------------------------------------------------------------------------
void VulkanRenderer::recordCommandBuffer()
{
    

    // Setup Color Attachment
    auto swapChainImageSize = m_window->swapChainImageSize();

    VkClearValue clearValues[2];
    clearValues[0].color = VkClearColorValue{ .float32 = {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = VkClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 };
    
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_window->defaultRenderPass(),
        .framebuffer = m_window->currentFramebuffer(),
        .renderArea = {
            .offset = {0, 0},
            .extent = {.width = static_cast<uint32_t>(swapChainImageSize.width()), .height = static_cast<uint32_t>(swapChainImageSize.height())}
        },
        .clearValueCount = 2,
        .pClearValues = clearValues
    };

    auto commandBuffer = m_window->currentCommandBuffer();
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    devFuncs->vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    devFuncs->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipeline);

    VkViewport viewPort = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapChainImageSize.width()),
        .height = static_cast<float>(swapChainImageSize.height()),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    devFuncs->vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);

    VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = {.width = static_cast<uint32_t>(swapChainImageSize.width()), .height = static_cast<uint32_t>(swapChainImageSize.height())}
    };
    devFuncs->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Draw
    devFuncs->vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    devFuncs->vkCmdEndRenderPass(commandBuffer);
}

//----------------------------------------------------------------------------------
void VulkanRenderer::transitionImageLayout(VkCommandBuffer commandBuffer,
                                           VkImageLayout oldLayout, 
                                           VkImageLayout newLayout,
                                           VkAccessFlags2 srcAccessMask,
                                           VkAccessFlags2 dstAccessMask,
                                           VkPipelineStageFlags2 srcStageMask,
                                           VkPipelineStageFlags2 dstStageMask)
{
    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_window->swapChainImage(m_window->currentSwapChainImageIndex()),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    auto vkCmdPipelinBarrier2 = 
        reinterpret_cast<PFN_vkCmdPipelineBarrier2>(m_window->vulkanInstance()->getInstanceProcAddr("vkCmdPipelineBarrier2"));
    
    if(!vkCmdPipelinBarrier2) 
    {
        qWarning() << "Failed to get function pointer for vkCmdPipelineBarrier2";
        throw std::runtime_error("Failed to get function pointer for vkCmdPipelineBarrier2");
    }

    vkCmdPipelinBarrier2(commandBuffer, &dependencyInfo);
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

    if ((shaderCode.size() % 4) != 0)
    {
        qWarning() << "Invalid SPIR-V size (must be multiple of 4):" << shaderCode.size() << "for" << filePath;
        return VK_NULL_HANDLE;
    }

    std::vector<uint32_t> spirv(shaderCode.size() / sizeof(uint32_t));
    std::memcpy(spirv.data(), shaderCode.constData(), static_cast<size_t>(shaderCode.size()));

    constexpr uint32_t kSpirvMagic = 0x07230203;
    if (spirv.empty() || spirv[0] != kSpirvMagic)
    {
        qWarning() << "Invalid SPIR-V magic in" << filePath
                   << "(did shader compilation emit Vulkan SPIR-V?)";
        return VK_NULL_HANDLE;
    }

    auto bytesRead = shaderCode.size();
    auto kilobytesRead = bytesRead / 1024.0;
    qInfo() << "Read" << bytesRead << "bytes (" << kilobytesRead << " KB) from shader file:" << filePath;

    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<size_t>(shaderCode.size()),
        .pCode = spirv.data()
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