#include "App.h"
#include <fstream>
//#define TINYOBJLOADER_IMPLEMENTATION
//#include "tiny_obj_loader.h"
#include <SDL2/SDL_vulkan.h>
#include "Mesh.h"


void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
    
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void App::cleanup()
{

    delete mTexture;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        //vkDestroyBuffer(_device._device, uniformBuffers[i], nullptr);
        //_device.destroyBuffer( uniformBuffers[i] );
        //_device.freeMemory( uniformBuffersMemory[i] );
        //vkFreeMemory(_device._device, uniformBuffersMemory[i], nullptr);
        delete uniformBuffers[i];
    }

    
    //vkDestroyDescriptorPool(_device._device, descriptorPool, nullptr);
    _device.destroyDescriptorPool( descriptorPool );

    //vkDestroyDescriptorSetLayout(_device._device, descriptorSetLayout, nullptr);
    _device.destroyDescriptorSetLayout( descriptorSetLayout );

    delete mesh;
    delete mesh2;

    _device.destroyPipeline( graphicsPipeline );
    _device.destroyPipelineLayout( pipelineLayout );


    //vkDestroyRenderPass(_device._device, renderPass, nullptr);
    _device.destroyRenderPass( renderPass );

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _device.destroySemaphore( renderFinishedSemaphores[i] );
        _device.destroySemaphore( imageAviablesSemaphores[i] );
        _device.destroyFence( inFlightFences[i] );
       /* vkDestroySemaphore(_device._device,renderFinishedSemaphores[i],nullptr);
        vkDestroySemaphore(_device._device, imageAviablesSemaphores[i], nullptr);
        vkDestroyFence(_device._device, inFlightFences[i], nullptr);*/
    }

    _device.destroyCommandPool( commandPool );
    _device.destroyCommandPool( transferPool );

    delete msaaTexture;
    delete depthTexture;
    delete normalTexture;

    for (auto framebuffer : swapChainFramebuffers) {
        _device.destroyFrameBuffer( framebuffer );
    }

    _window.close();

    _device.close();

    //vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {

        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    //glfwTerminate();
    vkDestroyInstance(instance, nullptr);

}

void App::createInstance() {



    //glfwSetErrorCallback(ErrorCallback);
    //glfwInit();
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    //SDL_Window* window = SDL_CreateWindow( "Ventanuco", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN );


    //_window.init( "Ventanuco", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, NULL);

    //info de la aplicacion
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Proyecto Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "LL Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;



    
    //informacion de la creacion de la instancia
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    //recuento de extensiones
    //uint32_t extensionCount = 0;
    //vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    //std::vector<VkExtensionProperties> extensions(extensionCount);

    //vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    //std::cout << "available extensions:\n";

    //for (const auto& extension : extensions) {
    //    std::cout << '\t' << extension.extensionName << '\n';
    //}


    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    std::vector<const char*> extensionsN = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsN.size());
    createInfo.ppEnabledExtensionNames = extensionsN.data();

    //Enable debug info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    //creacion de la instancia de vulkan
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }


    //_window.createSurface( _instance );
}

std::vector<const char*> App::getRequiredExtensions() {
    uint32_t SDLExtensionCount = 0;

    //TODO acceder a la ventana
    if (SDL_Vulkan_GetInstanceExtensions( nullptr, &SDLExtensionCount, NULL ) == SDL_bool::SDL_FALSE) {
        throw std::runtime_error( SDL_GetError() );
    }
    std::vector<const char*> extensions(SDLExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(nullptr,&SDLExtensionCount,extensions.data());

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

    return extensions;
}

void App::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}


//void App::createImageViews()
//{
//    swapChainImageViews.resize(swapChainImages.size());
//    for (size_t i = 0; i < swapChainImages.size(); i++) {
//        swapChainImageViews[i] = _device.createImageView(swapChainImages[i], _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT,1);
//    }
//}

void App::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("./vertex");
    auto fragShaderCode = readFile("./fragment");

    VkShaderModule vertShaderModule = _device.createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = _device.createShaderModule(fragShaderCode);


    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    //Configuracion de los vertices
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    


    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    //determina si la geometria pasa por el estado de rasterizado
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    //poligon mode de openGL
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    //lineWidth de openGL
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    //Configuracion del blendign de un buffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    //Configuracion del blendign de un buffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment2{};
    colorBlendAttachment2.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment2.blendEnable = VK_FALSE;
    colorBlendAttachment2.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment2.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment2.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment2.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment2.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment2.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendAttachmentState colorAttachments[] = { colorBlendAttachment,colorBlendAttachment2 };

    //Configuracion del blendig global
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 2;
    colorBlending.pAttachments = colorAttachments;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    ////Estado que nos permitia ajustar el viewport y el recortado del muestreo
    //VkPipelineViewportStateCreateInfo viewportState{};
    //viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    //viewportState.viewportCount = 1;
    //viewportState.pViewports = &viewport;
    //viewportState.scissorCount = 1;
    //viewportState.pScissors = &scissor;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional

    pipelineLayout = _device.createPipelineLayout( pipelineLayoutInfo );

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    

    //if (vkCreateGraphicsPipelines(_device._device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create graphics pipeline!");
    //}


    graphicsPipeline = _device.createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];
    

    _device.destroyShaderModule( fragShaderModule );
    _device.destroyShaderModule( vertShaderModule );
}

void App::createFramebuffers()
{
    swapChainFramebuffers.resize(_window.getImageViews().size());

    for (size_t i = 0; i < _window.getImageViews().size(); i++) {

        //el orden debe coincidir con la especificacion realizada en la descripcion de la pipeline
        std::array<VkImageView, 3> attachments = {
            _window.getImageViews()[i],
            depthTexture->textureImageView,
            normalTexture->textureImageView,

            //msaaTexture->textureImageView,
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _window.getExtent().width;
        framebufferInfo.height = _window.getExtent().height;
        framebufferInfo.layers = 1;

        //if (vkCreateFramebuffer(_device._device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
        //    throw std::runtime_error("failed to create framebuffer!");
        //}

        swapChainFramebuffers[i] = _device.createFrameBuffer( framebufferInfo );
    }
}

void App::createCommandPool()
{
    VulkanDevice::QueueFamilyIndices queueFamilyIndices = _device.getFamilyIndexes();

    commandPool= _device.createCommandPool( VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndices.graphicsFamily.value() );

    transferPool = _device.createCommandPool( VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndices.transferFamily.value() );
}

void App::createCommandBuffers()
{
    //commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //VkCommandBufferAllocateInfo allocInfo{};
    //allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //allocInfo.commandPool = commandPool;
    //allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    //if (vkAllocateCommandBuffers(_device._device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to allocate command buffers!");
    //}

    commandBuffers =  _device.createCommandBuffers( commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT );
    
}

void App::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional
    //empezamos a recibir datos para el command buffer
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = _window.getExtent();

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = { 0.0f, 0.0f,0.0f };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    //tomamos los datos de paso de renderizado
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    //Aplicamos la pipeline definida
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    //Llamadas a los estados dinamicos
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_window.getExtent().width);
    viewport.height = static_cast<float>(_window.getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = _window.getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    //VkBuffer vertexBuffers[] = { vertexBuffer };
    //VkDeviceSize offsets[] = { 0,0};
    //vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    ////ojo cuidao que como cambie el tamaño de los bits de un indice cagamos
    ////vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    //
    ////command buffer,cantidad de vertices,cantidad de instancias,offset de vertices, offset de instancias
    //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1 , 0, 0);



    //VkBuffer vertexBuffers2[] = { vertexBuffer2 };
    //VkDeviceSize offsets2[] = { 0 };
    //vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers2, offsets2 );

    //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices2.size()), 1, 0, 0);
    

    //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    mesh->draw( commandBuffer );
    mesh2->draw( commandBuffer );

    //vkCmdNextSubpass( commandBuffer, VK_SUBPASS_CONTENTS_INLINE );

    //vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline );

    //vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    //vkCmdDraw( commandBuffer, 3, 1, 0, 0 );

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

/*
* Outline of a frame
At a high level, rendering a frame in Vulkan consists of a common set of steps:

Wait for the previous frame to finish
Acquire an image from the swap chain
Record a command buffer which draws the scene onto that image
Submit the recorded command buffer
Present the swap chain image
*/
void App::drawFrame()
{
    //vkWaitForFences(_device._device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    _device.waitForFences( inFlightFences[currentFrame] );
    uint32_t imageIndex;
    //VkResult result = vkAcquireNextImageKHR(_device._device, swapChain, UINT64_MAX, imageAviablesSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    VkResult result = _device.adquireNextImage( _window.getSwapChain(), imageAviablesSemaphores[currentFrame], imageIndex);


    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    //vkResetFences(_device._device, 1, &inFlightFences[currentFrame]);
    updateUniformBuffer(currentFrame);
    _device.resetFences( inFlightFences[currentFrame] );

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);


    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore waitSemaphores[] = { imageAviablesSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { _window.getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR||_framebufferResized)
    {
        _framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

void App::loadModel()
{

    std::vector<Vertex> vertices;
    std::vector<Vertex> vertices2;

    Vertex v1;
    Vertex v2;
    Vertex v3;

    int dim = 1;

    v1.pos = glm::vec3( -dim, 0, 0);
    v2.pos = glm::vec3( 0, dim, 0 );
    v3.pos = glm::vec3( dim, 0, 0);

    v1.color = glm::vec3( 1.f, 0.0f, 0.f);
    v2.color = glm::vec3( 0.f, 1.f, 0.f);
    v3.color = glm::vec3( 0.f, 0.f, 1.f);
    
    v1.texCoord = glm::vec2(0,0);
    v2.texCoord = glm::vec2(2,0);
    v3.texCoord = glm::vec2(0,2);

    vertices.push_back( v1 );
    vertices.push_back( v2 );
    vertices.push_back( v3 );

    mesh = new Mesh( _device, vertices );

    v1.pos = glm::vec3( -dim*2, 0, 0.5f );
    v2.pos = glm::vec3( 0, dim, -1 );
    v3.pos = glm::vec3( dim*2, 0, 0 );

    v1.color = glm::vec3( 1.f, 0.0f, 0.f );
    v2.color = glm::vec3( 0.f, 1.f, 0.f );
    v3.color = glm::vec3( 0.f, 0.f, 1.f );

    v1.texCoord = glm::vec2( 0, 0 );
    v2.texCoord = glm::vec2( 2, 0 );
    v3.texCoord = glm::vec2( 0, 2 );

    vertices2.push_back( v1 );
    vertices2.push_back( v2 );
    vertices2.push_back( v3 );

    mesh2 = new Mesh( _device, MODEL_PATH );

}

void App::createSyncObjects()
{
    imageAviablesSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        imageAviablesSemaphores[i]=_device.createSemaphore( semaphoreInfo );
        renderFinishedSemaphores[i] = _device.createSemaphore( semaphoreInfo );
        inFlightFences[i] = _device.createFence( fenceInfo );
        //if (vkCreateSemaphore(_device._device, &semaphoreInfo, nullptr, &imageAviablesSemaphores[i]) != VK_SUCCESS ||
        //    vkCreateSemaphore(_device._device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        //    vkCreateFence(_device._device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
        //    throw std::runtime_error("failed to create semaphores!");
        //}
    }
}

void App::recreateSwapChain()
{
    //int width = 0, height = 0;
    //glfwGetFramebufferSize(_window.window, &width, &height);
    //
    //while (width == 0 || height == 0) {
    //    glfwGetFramebufferSize(_window.window, &width, &height);
    //    glfwWaitEvents();
    //}

    //vkDeviceWaitIdle(_device._device);
    _device.wait();

    _window.cleanUpSwapChain();

    _window.createSwapChain();
    createColorResources();
    createDepthResources();
    createFramebuffers();
}

void App::cleanupSwapChain()
{
    delete depthTexture;
    delete msaaTexture;

    for (auto framebuffer : swapChainFramebuffers) {
        _device.destroyFrameBuffer( framebuffer );
    }

    //for (auto imageView : swapChainImageViews) {
    //    _device.destroyImageView( imageView );
    //}

    //_device.destroySwapchain( swapChain );

    _window.cleanUpSwapChain();
}

void App::createColorResources()
{
    VkFormat colorFormat = _window.getFormat();

    msaaTexture = new Texture(_device);

    msaaTexture->createImage(_window.getExtent().width, _window.getExtent().height, 1, _device.getMssaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    msaaTexture->createImageView(colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    //VkFormat colorFormat = _window.getFormat();
    colorTexture = new Texture( _device );
    colorTexture->createImage( _window.getExtent().width, _window.getExtent().height, 1, _device.getMssaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    colorTexture->createImageView( colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
}

void App::createNormalResources()
{

    VkFormat colorFormat = _window.getFormat();
    normalTexture = new Texture( _device );
    normalTexture->createImage( _window.getExtent().width, _window.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    normalTexture->createImageView( colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
}

void App::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(0, 0, -2.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(90.0f), _window.getExtent().width / (float)_window.getExtent().height, 0.1f, 10.0f);
    //arreglo hecho debido a la logica invertida del eje y
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void App::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    //color
    VkDescriptorSetLayoutBinding input1{};
    input1.binding = 2;
    input1.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    input1.descriptorCount = 1;
    input1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    //normal
    VkDescriptorSetLayoutBinding input2{};
    input2.binding = 3;
    input2.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    input2.descriptorCount = 1;
    input2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding, input1, input2 };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    //if (vkCreateDescriptorSetLayout(_device._device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create descriptor set layout!");
    //}

    descriptorSetLayout = _device.createDescriptorSetLayout( layoutInfo );



    std::vector<VkDescriptorSetLayoutBinding> bindings2 = { input1, input2 };
    VkDescriptorSetLayoutCreateInfo deferredLayout{};
    deferredLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    deferredLayout.bindingCount = static_cast<uint32_t>(bindings2.size());
    deferredLayout.pBindings = bindings2.data();

    deferredDescriptorSet = _device.createDescriptorSetLayout( deferredLayout );

}


VkFormat App::findDepthFormat()
{
    return _device.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool App::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void App::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        //createBuffer(_device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
        uniformBuffers[i] = _device.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


        //vkMapMemory(_device._device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        _device.mapMemory( uniformBuffers[i]->getMemory(), 0, bufferSize, &uniformBuffersMapped[i]);
    }
}

void App::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)*2;


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    //if (vkCreateDescriptorPool(_device._device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create descriptor pool!");
    //}

    descriptorPool = _device.createDescriptorPool( poolInfo );
}

void App::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    depthTexture = new Texture(_device);

    depthTexture->createImage( _window.getExtent().width, _window.getExtent().height,1,VK_SAMPLE_COUNT_1_BIT, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //depthTexture->textureImageView = _device.createImageView(depthTexture->textureImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,1);
    depthTexture->createImageView( depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1 );

    //trasitionImageLayout(depthTexture->textureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,1);


    /*createImage(_swapChainExtent.width, _swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat);*/
}


void App::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    std::vector<std::vector<VkWriteDescriptorSet>> descriptorWritesVec;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        std::vector<VkWriteDescriptorSet> descriptorWrites(4);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i]->getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = mTexture->textureImageView;
        imageInfo.sampler = mTexture->textureSampler;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        VkDescriptorImageInfo inputInfo{};
        inputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputInfo.imageView = colorTexture->textureImageView;
        inputInfo.sampler = VK_NULL_HANDLE;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &inputInfo;

        VkDescriptorImageInfo inputInfo2{};
        inputInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputInfo2.imageView = normalTexture->textureImageView;
        inputInfo2.sampler = VK_NULL_HANDLE;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = &inputInfo2;

        //vkUpdateDescriptorSets(_device._device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        descriptorWritesVec.push_back( std::move( descriptorWrites ) );
    }

    descriptorSets = _device.createDescriptorSets( descriptorWritesVec, layouts, descriptorPool );
}

void App::generateMipmaps(VkImage image,VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{

    //// Check if image format supports linear blitting
    //VkFormatProperties formatProperties;
    //vkGetPhysicalDeviceFormatProperties(_device._physicalDevice, imageFormat, &formatProperties);

    //if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    //    throw std::runtime_error("texture image format does not support linear blitting!");
    //}

    //VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    //VkImageMemoryBarrier barrier{};
    //barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //barrier.image = image;
    //barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //barrier.subresourceRange.baseArrayLayer = 0;
    //barrier.subresourceRange.layerCount = 1;
    //barrier.subresourceRange.levelCount = 1;

    //int32_t mipWidth = texWidth;
    //int32_t mipHeight = texHeight;

    //for (uint32_t i = 1; i < mipLevels; i++) {
    //    barrier.subresourceRange.baseMipLevel = i - 1;
    //    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    //    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    //    vkCmdPipelineBarrier(commandBuffer,
    //        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
    //        0, nullptr,
    //        0, nullptr,
    //        1, &barrier);

    //    VkImageBlit blit{};
    //    blit.srcOffsets[0] = { 0, 0, 0 };
    //    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    //    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //    blit.srcSubresource.mipLevel = i - 1;
    //    blit.srcSubresource.baseArrayLayer = 0;
    //    blit.srcSubresource.layerCount = 1;
    //    blit.dstOffsets[0] = { 0, 0, 0 };
    //    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
    //    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //    blit.dstSubresource.mipLevel = i;
    //    blit.dstSubresource.baseArrayLayer = 0;
    //    blit.dstSubresource.layerCount = 1;

    //    vkCmdBlitImage(commandBuffer,
    //        image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //        image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //        1, &blit,
    //        VK_FILTER_LINEAR);

    //    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    //    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    //    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    //    vkCmdPipelineBarrier(commandBuffer,
    //        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    //        0, nullptr,
    //        0, nullptr,
    //        1, &barrier);

    //    if (mipWidth > 1) mipWidth /= 2;
    //    if (mipHeight > 1) mipHeight /= 2;

    //}

    //barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    //barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    //vkCmdPipelineBarrier(commandBuffer,
    //    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    //    0, nullptr,
    //    0, nullptr,
    //    1, &barrier);

    //endSingleTimeCommands(commandBuffer);
}

void App::trasitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = _device.beginSingleTimeCommands(commandPool);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    //si es a operacion de copia de la imagen
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    //si es la transicion de imagen a sample de shader
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    //si es la transicion de imagen para crear la stencil y depth
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }


    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );


    _device.endSingleTimeCommands(commandPool, commandBuffer,graphicsQueue);


}

void App::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer =_device.beginSingleTimeCommands(transferPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    _device.endSingleTimeCommands(transferPool,commandBuffer,trasferQueue);
}

void App::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _window.getFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT /*_device.msaaSamples*/;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription colorAttachment2{};
    colorAttachment2.format = _window.getFormat();
    colorAttachment2.samples = VK_SAMPLE_COUNT_1_BIT /*_device.msaaSamples*/;
    colorAttachment2.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment2.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment2.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment2.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment2.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    //VkAttachmentDescription colorAttachmentResolve{};
    //colorAttachmentResolve.format = _swapChainImageFormat;
    //colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    //colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference normlaAttachmentRef{};
    normlaAttachmentRef.attachment = 2;
    normlaAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference inputReference{};
    inputReference.attachment = 1;
    inputReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inputReference2{};
    inputReference2.attachment = 2;
    inputReference2.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


    //VkAttachmentReference colorAttachmentResolveRef{};
    //colorAttachmentResolveRef.attachment = 2;
    //colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference att[] = {colorAttachmentRef,normlaAttachmentRef};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 2;
    subpass.pColorAttachments = att;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    //subpass.pResolveAttachments = &colorAttachmentResolveRef;


    VkAttachmentReference att2[] = { inputReference,inputReference2 };

    VkSubpassDescription lightingSubPass{};
    lightingSubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    lightingSubPass.colorAttachmentCount = 1;
    lightingSubPass.pColorAttachments = &colorAttachmentRef;
    lightingSubPass.pInputAttachments = att2;
    lightingSubPass.inputAttachmentCount = 2;
    


    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment,colorAttachment2/*,colorAttachmentResolve*/ };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    //if (vkCreateRenderPass(_device._device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create render pass!");
    //}

    renderPass = _device.createRenderPass( renderPassInfo );
}

//void App::framebufferResizeCallback(GLFWwindow* window, int width, int height)
//{
//    auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
//    app->_framebufferResized = true;
//}

App::App()
{
   /* _window.init("Proyecto Vulkan",WIDTH,HEIGHT,_instance);*/
}


void App::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {

    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

}

bool App::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}