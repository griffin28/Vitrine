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

namespace myvulkan 
{
//----------------------------------------------------------------------------------
void VulkanRenderer::initResources() 
{
    // Create shader module
    const QString appDir = qApp->applicationDirPath(); 
    const QString fragPath = QDir(appDir).filePath("shaders/frag.spv");
    const QString vertPath = QDir(appDir).filePath("shaders/vert.spv");

    auto vertShaderModule = this->createShaderModule(vertPath);
    auto fragShaderModule = this->createShaderModule(fragPath);

    // Shader stage creation
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };
    shaderStages.push_back(vertexShaderStageInfo);

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };
    shaderStages.push_back(fragmentShaderStageInfo);

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
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
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
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkResult result = devFuncs->vkCreatePipelineLayout(m_window->device(), &pipelineLayoutInfo, nullptr, m_pipelineLayout);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create pipeline layout, VkResult:" << result;
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
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

    // Create vertex buffer
    this->createVertexBuffer();

    // Create index buffer
    this->createIndexBuffer();
}

//----------------------------------------------------------------------------------
void VulkanRenderer::releaseResources() 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    if(devFuncs->vkDeviceWaitIdle(m_window->device()) != VK_SUCCESS)
    {
        qWarning() << "Failed to wait for device idle";
    }

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

    // Vertex Buffer
    if(m_vertexBuffer) 
    {
        devFuncs->vkDestroyBuffer(m_window->device(), *m_vertexBuffer, nullptr);
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;   
    }

    if(m_vertexBufferMemory) 
    {
        devFuncs->vkFreeMemory(m_window->device(), *m_vertexBufferMemory, nullptr);
        delete m_vertexBufferMemory;
        m_vertexBufferMemory = nullptr;
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

    this->recordCommandBuffer(m_window->currentFrame());

    // QVulkanWindow performs acquire/submit/present and frame sync internally.
    m_window->frameReady();
    m_window->requestUpdate();
}

//----------------------------------------------------------------------------------
void VulkanRenderer::recordCommandBuffer(const int frameIndex)
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

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
    devFuncs->vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    devFuncs->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipeline);

    const VkBuffer vertexBuffers[] = {*m_vertexBuffer};
    const VkDeviceSize offsets[] = {0};
    devFuncs->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    devFuncs->vkCmdBindIndexBuffer(commandBuffer, *m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

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
    // devFuncs->vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    devFuncs->vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->getIndices().size()), 1, 0, 0, 0);

    devFuncs->vkCmdEndRenderPass(commandBuffer);
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

//----------------------------------------------------------------------------------
void VulkanRenderer::createIndexBuffer() 
{
    const auto indices = this->getIndices();

    if(indices.empty()) 
    {
        qWarning() << "Index list is empty, skipping index buffer creation."; 
        return;
    }

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkDeviceSize bufferSize = this->getIndexBufferSize();

    uint32_t memoryTypeIndex = m_window->hostVisibleMemoryIndex();
    if(memoryTypeIndex == UINT32_MAX)
    {
        qWarning() << "Failed to find suitable memory type for index buffer";
        return; 
    }

    // Staging Buffer Creation
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkResult result = this->createBuffer(bufferSize, 
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                         memoryTypeIndex, 
                                         stagingBuffer, 
                                         stagingBufferMemory);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create staging buffer for index data, VkResult:" << result;
        return;
    }

    void* data;
    result = devFuncs->vkMapMemory(m_window->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to map staging buffer memory for index data, VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return;
    }

    std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    devFuncs->vkUnmapMemory(m_window->device(), stagingBufferMemory);

    // Index Buffer Creation
    if(m_indexBuffer) 
    {
        devFuncs->vkDestroyBuffer(m_window->device(), *m_indexBuffer, nullptr);
        delete m_indexBuffer;   
    }

    if(m_indexBufferMemory) 
    {
        devFuncs->vkFreeMemory(m_window->device(), *m_indexBufferMemory, nullptr);
        delete m_indexBufferMemory;
    }

    m_indexBuffer = new VkBuffer;
    m_indexBufferMemory = new VkDeviceMemory;

    result = this->createBuffer(bufferSize, 
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                m_window->deviceLocalMemoryIndex(), 
                                *m_indexBuffer, 
                                *m_indexBufferMemory);
    if (result != VK_SUCCESS)    {
        qWarning() << "Failed to create index buffer, VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), *m_indexBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), *m_indexBufferMemory, nullptr);
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return;
    }

    // Copy from staging buffer to index buffer
    result = this->copyBuffer(stagingBuffer, *m_indexBuffer, bufferSize);
    if (result != VK_SUCCESS)    {
        qWarning() << "Failed to copy index data from staging buffer to index buffer, VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), *m_indexBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), *m_indexBufferMemory, nullptr);
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return;
    }

    devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
    devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
}

//----------------------------------------------------------------------------------
void VulkanRenderer::createVertexBuffer() 
{
    const auto vertices = this->getVertices();

    if(vertices.empty()) 
    {
        qWarning() << "Vertex list is empty, skipping vertex buffer creation."; 
        return;
    }

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    uint32_t memoryTypeIndex = m_window->hostVisibleMemoryIndex();
    if(memoryTypeIndex == UINT32_MAX)
    {
        qWarning() << "Failed to find suitable memory type for vertex buffer";
        return;
    }

    // Staging Buffer Creation
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    VkResult result = this->createBuffer(bufferSize, 
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                         memoryTypeIndex, 
                                         stagingBuffer, 
                                         stagingBufferMemory);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create staging buffer for vertex data, VkResult:" << result;
        return;
    }

    void* data;
    result = devFuncs->vkMapMemory(m_window->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to map staging buffer memory VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return;
    }

    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    devFuncs->vkUnmapMemory(m_window->device(), stagingBufferMemory);

    // Vertex Buffer Creation
    if(m_vertexBuffer) 
    {
        devFuncs->vkDestroyBuffer(m_window->device(), *m_vertexBuffer, nullptr);
        delete m_vertexBuffer;
    }
    m_vertexBuffer = new VkBuffer;

    if(m_vertexBufferMemory) 
    {
        devFuncs->vkFreeMemory(m_window->device(), *m_vertexBufferMemory, nullptr);
        delete m_vertexBufferMemory;
    }
    m_vertexBufferMemory = new VkDeviceMemory;

    result = this->createBuffer(bufferSize, 
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                m_window->deviceLocalMemoryIndex(), 
                                *m_vertexBuffer, 
                                *m_vertexBufferMemory);
    if (result != VK_SUCCESS)    {
        qWarning() << "Failed to create vertex buffer, VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), *m_vertexBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), *m_vertexBufferMemory, nullptr);
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return; 
    }

    // Copy from staging buffer to vertex buffer
    result = this->copyBuffer(stagingBuffer, *m_vertexBuffer, bufferSize);
    if (result != VK_SUCCESS)    {
        qWarning() << "Failed to copy vertex data from staging buffer to vertex buffer, VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), *m_vertexBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), *m_vertexBufferMemory, nullptr);
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return;
    }

    devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
    devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_window->graphicsCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    VkResult result = devFuncs->vkAllocateCommandBuffers(m_window->device(), &allocInfo, &commandBuffer);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to allocate command buffer for buffer copy, VkResult:" << result;
        return result;
    }

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    result = devFuncs->vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to begin command buffer for buffer copy, VkResult:" << result;
        devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
        return result;
    }

    VkBufferCopy copyRegion{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    devFuncs->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    result = devFuncs->vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to end command buffer for buffer copy, VkResult:" << result;
        devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
        return result;
    }

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    result = devFuncs->vkQueueSubmit(m_window->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        qWarning() << "Failed to submit command buffer for buffer copy, VkResult:" << result;
        devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
        return result;
    }

    devFuncs->vkQueueWaitIdle(m_window->graphicsQueue());
    devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
    return VK_SUCCESS;
}

//----------------------------------------------------------------------------------
uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, bool hostVisible) 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkPhysicalDeviceMemoryProperties memProperties;

    auto vkGetPhysicalDeviceMemoryPropertiesFunc = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(m_window->vulkanInstance()->getInstanceProcAddr("vkGetPhysicalDeviceMemoryProperties"));
    if (!vkGetPhysicalDeviceMemoryPropertiesFunc) 
    {
        qWarning() << "Failed to get function pointer for vkGetPhysicalDeviceMemoryProperties";
        throw std::runtime_error("Failed to get function pointer for vkGetPhysicalDeviceMemoryProperties");
    }

    if(hostVisible) 
    {
        auto hostVisibleMemoryIndex = m_window->hostVisibleMemoryIndex();
        if((typeFilter & (1 << hostVisibleMemoryIndex)) && (memProperties.memoryTypes[hostVisibleMemoryIndex].propertyFlags & properties) == properties) 
        {
            return hostVisibleMemoryIndex;
        }
    }
    else
    {
        auto deviceLocalMemoryIndex = m_window->deviceLocalMemoryIndex();
        if((typeFilter & (1 << deviceLocalMemoryIndex)) && (memProperties.memoryTypes[deviceLocalMemoryIndex].propertyFlags & properties) == properties) 
        {
            return deviceLocalMemoryIndex;
        }
    }

    vkGetPhysicalDeviceMemoryPropertiesFunc(m_window->physicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
        {
            return i;
        }
    }

    return UINT32_MAX; // Failed to find suitable memory type
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::createBuffer(VkDeviceSize size, 
                                      VkBufferUsageFlags usage, 
                                      VkMemoryPropertyFlags properties, 
                                      uint32_t memoryTypeIndex, 
                                      VkBuffer& buffer, 
                                      VkDeviceMemory& bufferMemory) 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (devFuncs->vkCreateBuffer(m_window->device(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) 
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkMemoryRequirements memRequirements;
    devFuncs->vkGetBufferMemoryRequirements(m_window->device(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    if (devFuncs->vkAllocateMemory(m_window->device(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) 
    {
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    return devFuncs->vkBindBufferMemory(m_window->device(), buffer, bufferMemory, 0);
}
}  // namespace myvulkan