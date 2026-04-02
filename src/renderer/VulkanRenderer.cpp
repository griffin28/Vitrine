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
#include <chrono>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
void VulkanRenderer::initResources() 
{
    // Multisampling
    // setSampleCount() must take effect before QVulkanWindow builds the default render
    // pass and framebuffers.  If the desired count differs from the current one, request
    // the change here and return immediately: QVulkanWindow will reset the swap chain and
    // invoke initResources() again, by which point defaultRenderPass() will already carry
    // the correct sample count and attachment layout.
    const VkSampleCountFlagBits msaaSamples = m_window->sampleCountFlagBits();
    // if (msaaSamples != m_window->sampleCountFlagBits())
    // {
    //     m_window->setSampleCount(static_cast<int>(msaaSamples));
    //     return;
    // }

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
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f,
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = msaaSamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // Depth and Stencil Testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
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

    // Uniforms and Push Constants
    VkDescriptorSetLayoutBinding uboLayoutBinding = UniformBufferObject::getDescriptorSetLayoutBinding(0);
    VkDescriptorSetLayoutBinding samplerBinding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerBinding};

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    m_descriptorSetLayout = new VkDescriptorSetLayout;
    
    VkResult result = devFuncs->vkCreateDescriptorSetLayout(m_window->device(), &descriptorSetLayoutInfo, nullptr, m_descriptorSetLayout);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create descriptor set layout, VkResult:" << result;
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = m_descriptorSetLayout,
        .pushConstantRangeCount = 0
    };

    m_pipelineLayout = new VkPipelineLayout;
    result = devFuncs->vkCreatePipelineLayout(m_window->device(), &pipelineLayoutInfo, nullptr, m_pipelineLayout);
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

    // Create Texture Image
    const QString texturePath = QDir(appDir).filePath("textures/viking_room.png");
    m_textureImage = new VkImage;
    m_textureImageMemory = new VkDeviceMemory;

    result = this->createTextureImage(texturePath, *m_textureImage, *m_textureImageMemory);
    if (result != VK_SUCCESS)
    {        
        qWarning() << "Failed to create texture image, VkResult:" << result;
        // throw std::runtime_error("Failed to create texture image");
    }

    // Create Texture Image View
    m_textureImageView = new VkImageView;

    result = this->createImageView(*m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, *m_textureImageView);
    if (result != VK_SUCCESS)
    {
        qWarning() << "Failed to create texture image view, VkResult:" << result;
    }

    // Create Texture Sampler
    m_textureSampler = new VkSampler;
    result = this->createTextureSampler(*m_textureSampler);
    if (result != VK_SUCCESS)    {
        qWarning() << "Failed to create texture sampler, VkResult:" << result;
    }

    this->loadModel(QDir(appDir).filePath("models/viking_room.obj"));

    // Create Buffers
    this->createVertexBuffer();
    this->createIndexBuffer();
    this->createUniformBuffers();

    result = this->createDescriptorPool();
    if (result != VK_SUCCESS)
    {
        qWarning() << "Failed to create descriptor pool, VkResult:" << result;
    }

    result = this->createDescriptorSets();
    if (result != VK_SUCCESS)
    {
        qWarning() << "Failed to create descriptor sets, VkResult:" << result;
    }
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

    // Index Buffer
    if(m_indexBuffer) 
    {
        devFuncs->vkDestroyBuffer(m_window->device(), *m_indexBuffer, nullptr);
        delete m_indexBuffer;
        m_indexBuffer = nullptr;
    }

    if(m_indexBufferMemory) 
    {
        devFuncs->vkFreeMemory(m_window->device(), *m_indexBufferMemory, nullptr);
        delete m_indexBufferMemory;
        m_indexBufferMemory = nullptr;
    }

    // Uniform Buffers
    for (size_t i = 0; i < m_uniformBuffers.size(); i++) 
    {
        if (m_uniformBuffers[i]) 
        {
            devFuncs->vkDestroyBuffer(m_window->device(), *m_uniformBuffers[i], nullptr);
            delete m_uniformBuffers[i];
            m_uniformBuffers[i] = nullptr;
        }

        if (m_uniformBuffersMemory[i]) 
        {
            devFuncs->vkFreeMemory(m_window->device(), *m_uniformBuffersMemory[i], nullptr);
            delete m_uniformBuffersMemory[i];
            m_uniformBuffersMemory[i] = nullptr;
        }

        if (m_uniformBuffersMapped[i]) 
        {
            m_uniformBuffersMapped[i] = nullptr;
        }
    }

    // Descriptor Pool and Sets
    if (m_descriptorPool) 
    {
        devFuncs->vkDestroyDescriptorPool(m_window->device(), *m_descriptorPool, nullptr);
        delete m_descriptorPool;
        m_descriptorPool = nullptr;
    }

    if (m_descriptorSetLayout) 
    {
        devFuncs->vkDestroyDescriptorSetLayout(m_window->device(), *m_descriptorSetLayout, nullptr);
        delete m_descriptorSetLayout;
        m_descriptorSetLayout = nullptr;    
    }

    // Texture Resources
    if (m_textureImage) 
    {
        devFuncs->vkDestroyImage(m_window->device(), *m_textureImage, nullptr);
        delete m_textureImage;
        m_textureImage = nullptr;
    }

    if (m_textureImageMemory) 
    {
        devFuncs->vkFreeMemory(m_window->device(), *m_textureImageMemory, nullptr);
        delete m_textureImageMemory;
        m_textureImageMemory = nullptr;
    }

    if (m_textureImageView) 
    {
        devFuncs->vkDestroyImageView(m_window->device(), *m_textureImageView, nullptr);
        delete m_textureImageView;
        m_textureImageView = nullptr;
    }

    if (m_textureSampler) 
    {
        devFuncs->vkDestroySampler(m_window->device(), *m_textureSampler, nullptr);
        delete m_textureSampler;
        m_textureSampler = nullptr;
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

//-----------------------------------------------------------------------------
VkSampleCountFlagBits VulkanRenderer::getMaxUsableSampleCount()
{
    auto sampleCounts = m_window->supportedSampleCounts();
    int maxCount = 1;

    for(int count: sampleCounts)
    {
        if(count > maxCount)
        {
            maxCount = count;
        }
    }

    if (maxCount & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (maxCount & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (maxCount & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (maxCount & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (maxCount & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (maxCount & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
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

    this->updateUniformBuffer();
    this->recordCommandBuffer();

    // QVulkanWindow performs acquire/submit/present and frame sync internally.
    m_window->frameReady();
    // m_window->requestUpdate();
}

//----------------------------------------------------------------------------------
void VulkanRenderer::recordCommandBuffer()
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    // Setup Color Attachment
    auto swapChainImageSize = m_window->swapChainImageSize();
    uint32_t clearValuesCount = 2;

    if(m_window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT)
    {
        clearValuesCount = 3;
    }

    VkClearValue clearValues[clearValuesCount];
    for(uint32_t i=0; i<clearValuesCount; ++i)
    {
        if(i == 0 || i == 2)
        {
            clearValues[i].color = VkClearColorValue{ .float32 = {0.0f, 0.0f, 0.0f, 1.0f} };
        }
        else
        {
            clearValues[i].depthStencil = VkClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 };
        }
    }
    
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_window->defaultRenderPass(),
        .framebuffer = m_window->currentFramebuffer(),
        .renderArea = {
            .offset = {0, 0},
            .extent = {.width = static_cast<uint32_t>(swapChainImageSize.width()), .height = static_cast<uint32_t>(swapChainImageSize.height())}
        },
        .clearValueCount = clearValuesCount,
        .pClearValues = clearValues
    };

    auto commandBuffer = m_window->currentCommandBuffer();
    devFuncs->vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    devFuncs->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipeline);

    const VkBuffer vertexBuffers[] = {*m_vertexBuffer};
    const VkDeviceSize offsets[] = {0};
    devFuncs->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    devFuncs->vkCmdBindIndexBuffer(commandBuffer, *m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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
    devFuncs->vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelineLayout, 0, 1, &m_descriptorSets[m_window->currentFrame()], 0, nullptr);
    devFuncs->vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

    devFuncs->vkCmdEndRenderPass(commandBuffer);
}

//----------------------------------------------------------------------------------
void VulkanRenderer::updateUniformBuffer() 
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    // Update uniform buffer with transformation matrices or other data as needed.
    // This is where you would typically calculate the model-view-projection matrix and copy it to the uniform buffer.
    const int frameIndex = m_window->currentFrame();
    
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), // fovy
                                static_cast<float>(m_window->swapChainImageSize().width()) / static_cast<float>(m_window->swapChainImageSize().height()), // aspect ratio
                                0.1f, 10.0f); // near and far planes
    ubo.proj[1][1] *= -1;

    if (m_uniformBuffersMapped[frameIndex]) 
    {
        std::memcpy(m_uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
    }
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
    if(m_indices.empty()) 
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

    std::memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
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
    }

    devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
    devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::createTextureImage(const QString& texturePath,
                                            VkImage& textureImage,
                                            VkDeviceMemory& textureImageMemory) 
{
    // Load image data using stb_image
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath.toStdString().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) 
    {
        qWarning() << "Failed to load texture image";
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    // Create staging buffer and copy pixel data to it
    // Create Vulkan image with VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    // Transition image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    // Copy buffer data to image
    // Transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkResult result = this->createBuffer(imageSize, 
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                         m_window->hostVisibleMemoryIndex(), 
                                         stagingBuffer, 
                                         stagingBufferMemory);
    if (result != VK_SUCCESS)    
    {
        qWarning() << "Failed to create staging buffer for texture image, VkResult:" << result;
        stbi_image_free(pixels);
        return result;
    }

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    void* data;
    result = devFuncs->vkMapMemory(m_window->device(), stagingBufferMemory, 0, imageSize, 0, &data);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to map staging buffer memory VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        stbi_image_free(pixels);
        return result;
    }

    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    devFuncs->vkUnmapMemory(m_window->device(), stagingBufferMemory);
    stbi_image_free(pixels);

    result = this->createImage(static_cast<uint32_t>(texWidth), 
                               static_cast<uint32_t>(texHeight),
                               VK_FORMAT_R8G8B8A8_SRGB, 
                               VK_IMAGE_TILING_OPTIMAL, 
                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                               textureImage, 
                               textureImageMemory);

    if (result != VK_SUCCESS)
    {
        qWarning() << "Failed to create texture image, VkResult:" << result;
        return result;
    }

    result = this->transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (result != VK_SUCCESS)
    {
        qWarning() << "Failed to transition texture image layout to TRANSFER_DST_OPTIMAL, VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return result;
    }

    result = this->copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    if (result != VK_SUCCESS)    {
        qWarning() << "Failed to copy buffer to texture image, VkResult:" << result;
        devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
        return result;
    }

    // result = this->transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // if (result != VK_SUCCESS)    
    // {
    //     qWarning() << "Failed to transition texture image layout to SHADER_READ_ONLY_OPTIMAL, VkResult:" << result;
    // }

    // Generate mipmaps for the texture image
    result = this->generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, static_cast<int32_t>(texWidth), static_cast<int32_t>(texHeight), m_mipLevels);
    
    devFuncs->vkDestroyBuffer(m_window->device(), stagingBuffer, nullptr);
    devFuncs->vkFreeMemory(m_window->device(), stagingBufferMemory, nullptr);
    return result;
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::generateMipmaps(VkImage& image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties{};

    auto vkGetPhysicalDeviceFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(m_window->vulkanInstance()->getInstanceProcAddr("vkGetPhysicalDeviceFormatProperties"));
    if(!vkGetPhysicalDeviceFormatProperties)
    {
        qWarning() << "Failed to get function pointer for vkGetPhysicalDeviceFormatProperties";
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    vkGetPhysicalDeviceFormatProperties(m_window->physicalDevice(), imageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
    {
        qWarning() << "Texture image format does not support linear blitting!";
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) 
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        devFuncs->vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit blit{
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets = {
                {0, 0, 0},
                {mipWidth, mipHeight, 1}
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets = {
                {0, 0, 0},
                {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
            }
        };

        devFuncs->vkCmdBlitImage(
            commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        devFuncs->vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    devFuncs->vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    return endSingleTimeCommands(commandBuffer);
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::createImageView(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) 
{
     VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = m_mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkResult result = devFuncs->vkCreateImageView(m_window->device(), &viewInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create image view, VkResult:" << result;
        return result;
    }

    return VK_SUCCESS;
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::createTextureSampler(VkSampler& textureSampler) 
{
    const VkPhysicalDeviceProperties* deviceProperties = m_window->physicalDeviceProperties();
    VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = deviceProperties->limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkResult result = devFuncs->vkCreateSampler(m_window->device(), &samplerInfo, nullptr, &textureSampler);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create texture sampler, VkResult:" << result;
        return result;
    }

    return VK_SUCCESS;
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::createImage(uint32_t width,
                                     uint32_t height,
                                     VkFormat format,
                                     VkImageTiling tiling,
                                     VkImageUsageFlags usage,
                                     VkMemoryPropertyFlags properties,
                                     VkImage& image,
                                     VkDeviceMemory& imageMemory) 
{
    // Create a Vulkan image with the specified parameters and allocate memory for it.
    // This function is used for creating texture images, depth images, etc.
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = m_mipLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkResult result = devFuncs->vkCreateImage(m_window->device(), &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to create image, VkResult:" << result;
        return result; 
    }

    VkMemoryRequirements memRequirements;
    devFuncs->vkGetImageMemoryRequirements(m_window->device(), image, &memRequirements);

    uint32_t memoryTypeIndex = this->findMemoryType(memRequirements.memoryTypeBits, properties);
    if (memoryTypeIndex == UINT32_MAX)
    {
        qWarning() << "Failed to find suitable memory type for image";
        devFuncs->vkDestroyImage(m_window->device(), image, nullptr);
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    result = devFuncs->vkAllocateMemory(m_window->device(), &allocInfo, nullptr, &imageMemory);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to allocate image memory, VkResult:" << result;
        devFuncs->vkDestroyImage(m_window->device(), image, nullptr);
        return result;
    }

    result = devFuncs->vkBindImageMemory(m_window->device(), image, imageMemory, 0);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to bind image memory, VkResult:" << result;
        devFuncs->vkDestroyImage(m_window->device(), image, nullptr);
        devFuncs->vkFreeMemory(m_window->device(), imageMemory, nullptr);
        return result;
    }

    return VK_SUCCESS;
}

//----------------------------------------------------------------------------------
void VulkanRenderer::createVertexBuffer() 
{
    if(m_vertices.empty()) 
    {
        qWarning() << "Vertex list is empty, skipping vertex buffer creation."; 
        return;
    }

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
    
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

    std::memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));
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
void VulkanRenderer::createUniformBuffers() 
{
    m_uniformBuffers.clear();
    m_uniformBuffersMemory.clear();
    m_uniformBuffersMapped.clear();

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    for(size_t i = 0; i < QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT; i++) 
    {
        VkBuffer *buffer = new VkBuffer;
        VkDeviceMemory *bufferMemory = new VkDeviceMemory;
        VkResult result = this->createBuffer(bufferSize, 
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                             m_window->hostVisibleMemoryIndex(), 
                                             *buffer, 
                                             *bufferMemory);
        if (result != VK_SUCCESS) 
        {
            qWarning() << "Failed to create uniform buffer " << i << ", VkResult:" << result;
            return;
        }
        m_uniformBuffers.emplace_back(std::move(buffer));
        m_uniformBuffersMemory.emplace_back(std::move(bufferMemory));
        
        void *mappedData;
        result = devFuncs->vkMapMemory(m_window->device(), *m_uniformBuffersMemory[i], 0, bufferSize, 0, &mappedData);
        if (result != VK_SUCCESS) 
        {
            qWarning() << "Failed to map uniform buffer memory for buffer " << i << ", VkResult:" << result;
            devFuncs->vkDestroyBuffer(m_window->device(), *m_uniformBuffers[i], nullptr);
            devFuncs->vkFreeMemory(m_window->device(), *m_uniformBuffersMemory[i], nullptr);
            return;
        }
        m_uniformBuffersMapped.emplace_back(mappedData);
    }
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::createDescriptorPool() 
{
    // Create a descriptor pool that can allocate descriptor sets for our uniform buffers.
    // This is needed if we want to use descriptor sets to bind our uniform buffers to the pipeline.
    VkDescriptorPoolSize uniformPoolSize{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT)
    };

    VkDescriptorPoolSize samplerPoolSize{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT)
    };

    std::array<VkDescriptorPoolSize, 2> poolSizes = {uniformPoolSize, samplerPoolSize};


    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = static_cast<uint32_t>(QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    m_descriptorPool = new VkDescriptorPool;

    return devFuncs->vkCreateDescriptorPool(m_window->device(), &poolInfo, nullptr, m_descriptorPool);
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::createDescriptorSets() 
{
    // Allocate and configure descriptor sets for our uniform buffers.
    std::vector<VkDescriptorSetLayout> layouts(QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT, *m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = *m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    m_descriptorSets.clear();
    m_descriptorSets.resize(QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT);
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkResult result = devFuncs->vkAllocateDescriptorSets(m_window->device(), &allocInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) 
    {
        return result;
    }

    for (size_t i = 0; i < QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT; i++) 
    {
        VkDescriptorBufferInfo bufferInfo{
            .buffer = *m_uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        VkDescriptorImageInfo imageInfo{
            .sampler = *m_textureSampler,
            .imageView = *m_textureImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet descriptorWriteUniform{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo
        };

        VkWriteDescriptorSet descriptorWriteImage{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
        };

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {descriptorWriteUniform, descriptorWriteImage};
        devFuncs->vkUpdateDescriptorSets(m_window->device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();
    if (commandBuffer == VK_NULL_HANDLE)    {
        qWarning() << "Failed to begin single-time command buffer for buffer copy";
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkBufferCopy copyRegion{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    devFuncs->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    return this->endSingleTimeCommands(commandBuffer);
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::copyBufferToImage(const VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height) 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();
    if (commandBuffer == VK_NULL_HANDLE)    {
        qWarning() << "Failed to begin single-time command buffer for buffer-to-image copy";
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1
        }
    };

    devFuncs->vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    return this->endSingleTimeCommands(commandBuffer);
}

//----------------------------------------------------------------------------------
uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, bool hostVisible) 
{
    VkPhysicalDeviceMemoryProperties memProperties;

    auto vkGetPhysicalDeviceMemoryPropertiesFunc = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(m_window->vulkanInstance()->getInstanceProcAddr("vkGetPhysicalDeviceMemoryProperties"));
    if (!vkGetPhysicalDeviceMemoryPropertiesFunc) 
    {
        qWarning() << "Failed to get function pointer for vkGetPhysicalDeviceMemoryProperties";
        throw std::runtime_error("Failed to get function pointer for vkGetPhysicalDeviceMemoryProperties");
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

//----------------------------------------------------------------------------------
VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_window->graphicsCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    if (devFuncs->vkAllocateCommandBuffers(m_window->device(), &allocInfo, &commandBuffer) != VK_SUCCESS) 
    {
        qWarning() << "Failed to allocate command buffer for single-time commands";
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    if (devFuncs->vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
    {
        qWarning() << "Failed to begin command buffer for single-time commands";
        devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
        return VK_NULL_HANDLE;
    }

    return commandBuffer;
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::endSingleTimeCommands(VkCommandBuffer& commandBuffer) 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkResult result = devFuncs->vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) 
    {
        qWarning() << "Failed to end command buffer for single-time commands";
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
        qWarning() << "Failed to submit command buffer for single-time commands";
        devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
        return result;
    }

    devFuncs->vkQueueWaitIdle(m_window->graphicsQueue());
    devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
    return VK_SUCCESS;
}

//----------------------------------------------------------------------------------
VkResult VulkanRenderer::transitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout) 
{
    auto devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());

    VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();
    if (commandBuffer == VK_NULL_HANDLE) 
    {
        qWarning() << "Failed to begin single-time command buffer for image layout transition";
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = m_mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } 
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else 
    {
        qWarning() << "Unsupported layout transition from" << oldLayout << "to" << newLayout;
        devFuncs->vkFreeCommandBuffers(m_window->device(), m_window->graphicsCommandPool(), 1, &commandBuffer);
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    } 

    devFuncs->vkCmdPipelineBarrier(commandBuffer,
                                   sourceStage, destinationStage,
                                   0,
                                   0, nullptr,
                                   0, nullptr,
                                   1, &barrier
    );

    return this->endSingleTimeCommands(commandBuffer);
}

//-----------------------------------------------------------------------------
bool VulkanRenderer::hasStencilComponent(VkFormat format) 
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

//-----------------------------------------------------------------------------
void VulkanRenderer::loadModel(const QString& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.toStdString().c_str())) 
    {
        qWarning() << "Failed to load model:" << path << "\n" << warn.c_str() << "\n" << err.c_str();
        return;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    m_vertices.clear();
    m_indices.clear();

    for (const auto& shape : shapes) 
    {
        for (const auto& index : shape.mesh.indices) 
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            // if (index.normal_index >= 0) 
            // {
            //     vertex.normal = {
            //         attrib.normals[3 * index.normal_index + 0],
            //         attrib.normals[3 * index.normal_index + 1],
            //         attrib.normals[3 * index.normal_index + 2]
            //     };
            // }

            if (index.texcoord_index >= 0) 
            {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            vertex.color = {1.0f, 1.0f, 1.0f};

            if(uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }

            m_indices.push_back(uniqueVertices[vertex]);
        }
    }

    qInfo() << "Loaded model:" << path << "with" << m_vertices.size() << "vertices and" << m_indices.size() << "indices.";
}
}  // namespace myvulkan