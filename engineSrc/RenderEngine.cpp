#include "RenderEngine.h"
#include <fstream>
//#define TINYOBJLOADER_IMPLEMENTATION
//#include "tiny_obj_loader.h"
#include <SDL2/SDL_vulkan.h>
#include "Mesh.h"


void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator ) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if (func != nullptr) {
        func( instance, debugMessenger, pAllocator );
    }

}

void RenderEngine::wait() {
    _device.wait();
}

VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger ) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if (func != nullptr) {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


void RenderEngine::cleanup()
{

    for (Texture* texture : _textureArray) {
        delete texture;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        delete uniformBuffers[i];
    }

    delete _lightUniformBuffer;
    delete _lightBufferSorage;

    //vkDestroyDescriptorPool(_device._device, descriptorPool, nullptr);
    _device.destroyDescriptorPool( descriptorPool );

    //vkDestroyDescriptorSetLayout(_device._device, descriptorSetLayout, nullptr);
    _device.destroyDescriptorSetLayout( descriptorSetLayout );
    _device.destroyDescriptorSetLayout( deferredDescriptorSetLayout );

    _device.destroyPipeline( graphicsPipeline );
    _device.destroyPipeline( noTexPipeline );
    _device.destroyPipelineLayout( pipelineLayout );

    _device.destroyPipeline( deferredPipeline );
    _device.destroyPipelineLayout( deferredLayout );


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
    //_device.destroyCommandPool( transferPool );

    //delete msaaTexture;
    delete depthTexture;
    delete normalTexture;
    delete colorTexture;
    delete posTexture;

    for (auto framebuffer : swapChainFramebuffers) {
        _device.destroyFrameBuffer( framebuffer );
    }

    _window.close();

    _device.close();

    //vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {

        DestroyDebugUtilsMessengerEXT( instance, debugMessenger, nullptr );
    }
    //glfwTerminate();
    vkDestroyInstance( instance, nullptr );

}

void RenderEngine::createInstance() {



    //glfwSetErrorCallback(ErrorCallback);
    //glfwInit();
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error( "validation layers requested, but not available!" );
    }

    //SDL_Window* window = SDL_CreateWindow( "Ventanuco", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN );


    //_window.init( "Ventanuco", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, NULL);

    //info de la aplicacion
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Proyecto Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName = "LL Engine";
    appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
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

        populateDebugMessengerCreateInfo( debugCreateInfo );
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    //creacion de la instancia de vulkan
    if (vkCreateInstance( &createInfo, nullptr, &instance ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create instance!" );
    }


    //_window.createSurface( _instance );
}

std::vector<const char*> RenderEngine::getRequiredExtensions() {
    uint32_t SDLExtensionCount = 0;

    //TODO acceder a la ventana
    if (SDL_Vulkan_GetInstanceExtensions( nullptr, &SDLExtensionCount, NULL ) == SDL_bool::SDL_FALSE) {
        throw std::runtime_error( SDL_GetError() );
    }
    std::vector<const char*> extensions( SDLExtensionCount );
    SDL_Vulkan_GetInstanceExtensions( nullptr, &SDLExtensionCount, extensions.data() );

    if (enableValidationLayers) {
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }
    extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

    return extensions;
}

void RenderEngine::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo( createInfo );
    if (CreateDebugUtilsMessengerEXT( instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to set up debug messenger!" );
    }
}

void RenderEngine::createGraphicsPipeline()
{
    auto vertShaderCode = readFile( "./shaders/build/vertex" );
    auto fragShaderCode = readFile( "./shaders/build/fragment" );
    auto noTexShaderCode = readFile( "./shaders/build/fragmentNoTexture" );

    VkShaderModule vertShaderModule = _device.createShaderModule( vertShaderCode );
    VkShaderModule fragShaderModule = _device.createShaderModule( fragShaderCode );
    VkShaderModule noTexShaderModule = _device.createShaderModule( noTexShaderCode );


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

    VkPipelineShaderStageCreateInfo noTexShaderStageInfo{};
    noTexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    noTexShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    noTexShaderStageInfo.module = noTexShaderModule;
    noTexShaderStageInfo.pName = "main";

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
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    //Configuracion del blendign de un buffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment2{};
    colorBlendAttachment2.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment2.blendEnable = VK_TRUE;
    colorBlendAttachment2.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment2.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment2.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment2.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment2.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment2.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    //Configuracion del blendign de un buffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment3{};
    colorBlendAttachment3.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment3.blendEnable = VK_TRUE;
    colorBlendAttachment3.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment3.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment3.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment3.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment3.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment3.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendAttachmentState colorAttachments[] = { colorBlendAttachment,colorBlendAttachment2,colorBlendAttachment3 };

    //Configuracion del blendig global
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 3;
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

    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    range.size = sizeof( glm::mat4 );
    range.offset = 0;

    VkPushConstantRange range2{};
    range2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    range2.size = sizeof( MaterialData );
    range2.offset = sizeof( glm::mat4 );

    VkPushConstantRange ranges[] = { range,range2 };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 2;
    pipelineLayoutInfo.pPushConstantRanges = ranges;
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

    shaderStages[1] = noTexShaderStageInfo;

    noTexPipeline = _device.createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

    _device.destroyShaderModule( fragShaderModule );
    _device.destroyShaderModule( vertShaderModule );
    _device.destroyShaderModule( noTexShaderModule );
}

void RenderEngine::createDeferredPipeline()
{
    auto fragShaderCode = readFile( "./shaders/build/deferred" );
    auto vertShaderCode = readFile( "./shaders/build/readAttachmentVertex" );

    VkShaderModule vertShaderModule = _device.createShaderModule( vertShaderCode );
    VkShaderModule fragShaderModule = _device.createShaderModule( fragShaderCode );


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


    //Configuracion de los vertices
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;


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
    rasterizer.cullMode = VK_CULL_MODE_NONE;
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
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional


    VkPipelineColorBlendAttachmentState colorAttachments[] = { colorBlendAttachment };

    //Configuracion del blendig global
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = colorAttachments;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
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


    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &deferredDescriptorSetLayout; // Optional

    deferredLayout = _device.createPipelineLayout( pipelineLayoutInfo );

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
    pipelineInfo.layout = deferredLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 1;



    //if (vkCreateGraphicsPipelines(_device._device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create graphics pipeline!");
    //}


    deferredPipeline = _device.createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

    _device.destroyShaderModule( vertShaderModule );
    _device.destroyShaderModule( fragShaderModule );
}

void RenderEngine::createFramebuffers()
{
    swapChainFramebuffers.resize( _window.getImageViews().size() );

    for (size_t i = 0; i < _window.getImageViews().size(); i++) {

        //el orden debe coincidir con la especificacion realizada en la descripcion de la pipeline
        std::array<VkImageView, 5> attachments = {
            _window.getImageViews()[i],
            depthTexture->textureImageView,
            colorTexture->textureImageView,
            normalTexture->textureImageView,
            posTexture->textureImageView
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

void RenderEngine::createCommandPool()
{
    VulkanDevice::QueueFamilyIndices queueFamilyIndices = _device.getFamilyIndexes();

    commandPool = _device.createCommandPool( VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndices.graphicsFamily.value() );
    //transferPool = _device.createCommandPool( VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndices.transferFamily.value() );
}

void RenderEngine::createCommandBuffers()
{

    commandBuffers = _device.createCommandBuffers( commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT );

}

Mesh* RenderEngine::createMesh( Mesh& meshCopy )
{
    return new Mesh(meshCopy);
}

Mesh* RenderEngine::createMesh( const std::string& path )
{
    return new Mesh(_device,path);
}

Mesh* RenderEngine::createMesh( const std::vector<Vertex>& vertices )
{
    return new Mesh(_device,vertices);
}

Mesh* RenderEngine::createMesh( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices )
{
    return new Mesh(_device,indices,vertices);
}

void RenderEngine::createPointLight( glm::vec3 position, glm::vec3 color, float intensity )
{
    Light l;

    l.pos_dir = position;
    l.color = color;
    l.type = 1;
    l.intensity = intensity;

    _lightBuffer.push_back( l );

    updateLightBuffer();
}

void RenderEngine::createDirectionalLight( glm::vec3 direction, glm::vec3 color, float intensity )
{
    Light l;

    l.pos_dir = direction;
    l.color = color;
    l.type = 0;
    l.intensity = intensity;

    _lightBuffer.push_back( l );

    updateLightBuffer();
}

int RenderEngine::loadTexture( const std::string& path ) {
    
    Texture* tex = new Texture(_device);
    tex->loadTexture( path );

    _textureArray.push_back( tex);

    updateGeometryDescriptorSets();

    return _textureArray.size()-1;
}

void RenderEngine::recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex, std::vector<RenderObject>& objectsArray )
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional
    //empezamos a recibir datos para el command buffer
    if (vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to begin recording command buffer!" );
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = _window.getExtent();

    std::array<VkClearValue, 5> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[3].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
    clearValues[4].color = { {0.0f, 0.0f, 0.0f, 0.0f} };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    //tomamos los datos de paso de renderizado
    vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

    vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr );
    //Aplicamos la pipeline definida
    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );

    //Llamadas a los estados dinamicos
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_window.getExtent().width);
    viewport.height = static_cast<float>(_window.getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( commandBuffer, 0, 1, &viewport );

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = _window.getExtent();
    vkCmdSetScissor( commandBuffer, 0, 1, &scissor );



    for (auto& object : objectsArray) {

        pushTextureIndex( commandBuffer, object.mat );
        pushModelMatrix( commandBuffer, object.modelMatrix );

        object.mesh->draw( commandBuffer );
    }

    vkCmdNextSubpass( commandBuffer, VK_SUBPASS_CONTENTS_INLINE );

    vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLayout, 0, 1, &lightingDescriptorSets[currentFrame], 0, nullptr );

    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline );

    vkCmdDraw( commandBuffer, 3, 1, 0, 0 );

    vkCmdEndRenderPass( commandBuffer );

    if (vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to record command buffer!" );
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
void RenderEngine::drawFrame( std::vector<RenderObject>& objectsArray )
{
    //vkWaitForFences(_device._device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    _device.waitForFences( inFlightFences[currentFrame] );
    uint32_t imageIndex;
    //VkResult result = vkAcquireNextImageKHR(_device._device, swapChain, UINT64_MAX, imageAviablesSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    VkResult result = _device.adquireNextImage( _window.getSwapChain(), imageAviablesSemaphores[currentFrame], imageIndex );


    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        updateGeometryDescriptorSets();
        updateLightingDescriptorSets();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error( "failed to acquire swap chain image!" );
    }
    //vkResetFences(_device._device, 1, &inFlightFences[currentFrame]);
    updateUniformBuffer( currentFrame, glm::mat4( 1.0f ) );
    _device.resetFences( inFlightFences[currentFrame] );

    vkResetCommandBuffer( commandBuffers[currentFrame], 0 );
    recordCommandBuffer( commandBuffers[currentFrame], imageIndex, objectsArray );


    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAviablesSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit( graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame] ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to submit draw command buffer!" );
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { _window.getSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR( presentQueue, &presentInfo );

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized)
    {
        _framebufferResized = false;
        recreateSwapChain();

        updateGeometryDescriptorSets();
        updateLightingDescriptorSets();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error( "failed to present swap chain image!" );
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}


void RenderEngine::createSyncObjects()
{
    imageAviablesSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    renderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        imageAviablesSemaphores[i] = _device.createSemaphore( semaphoreInfo );
        renderFinishedSemaphores[i] = _device.createSemaphore( semaphoreInfo );
        inFlightFences[i] = _device.createFence( fenceInfo );
        //if (vkCreateSemaphore(_device._device, &semaphoreInfo, nullptr, &imageAviablesSemaphores[i]) != VK_SUCCESS ||
        //    vkCreateSemaphore(_device._device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        //    vkCreateFence(_device._device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
        //    throw std::runtime_error("failed to create semaphores!");
        //}
    }
}

void RenderEngine::recreateSwapChain()
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

    cleanupSwapChain();



    _window.createSwapChain();
    createColorResources();
    createDepthResources();
    createNormalResources();


    createFramebuffers();
}

void RenderEngine::cleanupSwapChain()
{
    delete depthTexture;
    delete colorTexture;
    delete normalTexture;
    delete posTexture;
    //delete msaaTexture;

    for (auto framebuffer : swapChainFramebuffers) {
        _device.destroyFrameBuffer( framebuffer );
    }

    //for (auto imageView : swapChainImageViews) {
    //    _device.destroyImageView( imageView );
    //}

    //_device.destroySwapchain( swapChain );

    _window.cleanUpSwapChain();
}

void RenderEngine::createColorResources()
{
    VkFormat colorFormat = _window.getFormat();

    //msaaTexture = new Texture(_device);

    //msaaTexture->createImage(_window.getExtent().width, _window.getExtent().height, 1, _device.getMssaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT| VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //msaaTexture->createImageView(colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    //VkFormat colorFormat = _window.getFormat();
    colorTexture = new Texture( _device );
    colorTexture->createImage( _window.getExtent().width, _window.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    colorTexture->createImageView( colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
}

void RenderEngine::createNormalResources()
{

    VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    normalTexture = new Texture( _device );
    normalTexture->createImage( _window.getExtent().width, _window.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    normalTexture->createImageView( colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );


    posTexture = new Texture( _device );
    posTexture->createImage( _window.getExtent().width, _window.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    posTexture->createImageView( colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
}

void RenderEngine::updateUniformBuffer( uint32_t currentImage, glm::mat4 model )
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

    UniformBufferObject ubo{};
    //ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = _mainCamera.getViewMatrix();//glm::lookAt(glm::vec3(0, 0, -2.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = _mainCamera.getProjMatrix(); //glm::perspective( glm::radians( 90.0f ), _window.getExtent().width / (float)_window.getExtent().height, 0.1f, 10.0f );
    //arreglo hecho debido a la logica invertida del eje y
    ubo.proj[1][1] *= -1;

    memcpy( uniformBuffersMapped[currentImage], &ubo, sizeof( ubo ) );
    _lighting.eyePos = _mainCamera.getPos();

    memcpy( _lightBufferMapped, &_lighting, sizeof( _lighting ) );
    //vkCmdUpdateBuffer( commandBuffers[currentImage], uniformBuffers[currentImage]->getBuffer(), 0, sizeof( UniformBufferObject ), uniformBuffersMapped[currentImage] );
}


void RenderEngine::createLightBuffer()
{
    _lighting.eyePos = glm::vec3( 0, 0, -2.5f );

    _lighting.ambietnVal = 0.01;

    VkDeviceSize memorySize = sizeof( int ) + (sizeof( Light ) * MAX_LIGHTS) + sizeof( uint8_t ) * 16;
    _lightBufferSorage = _device.createBuffer( memorySize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
    _device.mapMemory( _lightBufferSorage->getMemory(), 0, memorySize, &_lightBufferStorageMapped );

   
}

void RenderEngine::updateLightBuffer() {

    int size = _lightBuffer.size();

    memcpy( _lightBufferStorageMapped, &size, sizeof( int ) );

    //+16 por alineamietno en memoria en la gpu
    void* arrayStart = (uint8_t*)_lightBufferStorageMapped + 16;
    memcpy( arrayStart, _lightBuffer.data(), sizeof( Light ) * _lightBuffer.size() );
}

void RenderEngine::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 3;
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

    VkDescriptorSetLayoutBinding input3{};
    input3.binding = 5;
    input3.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    input3.descriptorCount = 1;
    input3.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    //if (vkCreateDescriptorSetLayout(_device._device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create descriptor set layout!");
    //}

    descriptorSetLayout = _device.createDescriptorSetLayout( layoutInfo );

    VkDescriptorSetLayoutBinding lightingBinding{};
    lightingBinding.binding = 4;
    lightingBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingBinding.descriptorCount = 1;
    lightingBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightingBinding.pImmutableSamplers = nullptr;


    VkDescriptorSetLayoutBinding lightsBinding{};
    lightsBinding.binding = 6;
    lightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightsBinding.descriptorCount = 1;
    lightsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightsBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> bindings2 = { uboLayoutBinding, input1, input2 ,input3, lightingBinding, lightsBinding };
    VkDescriptorSetLayoutCreateInfo deferredLayout{};
    deferredLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    deferredLayout.bindingCount = static_cast<uint32_t>(bindings2.size());
    deferredLayout.pBindings = bindings2.data();

    deferredDescriptorSetLayout = _device.createDescriptorSetLayout( deferredLayout );

}


VkFormat RenderEngine::findDepthFormat()
{
    return _device.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool RenderEngine::hasStencilComponent( VkFormat format )
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void RenderEngine::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof( UniformBufferObject );

    uniformBuffers.resize( MAX_FRAMES_IN_FLIGHT );
    //uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize( MAX_FRAMES_IN_FLIGHT );

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        //createBuffer(_device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
        uniformBuffers[i] = _device.createBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );


        //vkMapMemory(_device._device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        _device.mapMemory( uniformBuffers[i]->getMemory(), 0, bufferSize, &uniformBuffersMapped[i] );
    }

    //Creamos un buffer para los datos de la luz
    VkDeviceSize lightBufferSize = sizeof( GlobalLighting );


    _lightUniformBuffer = _device.createBuffer( lightBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
    _device.mapMemory( _lightUniformBuffer->getMemory(), 0, lightBufferSize, &_lightBufferMapped );
    memcpy( _lightBufferMapped, &_lighting, sizeof( _lighting ) );

}

void RenderEngine::createDescriptorPool()
{
    int numOfTextures = 3;

    int numOfInputAttachments = 3;

    int numOfUniformBuffres = 3;

    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * numOfUniformBuffres;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * numOfTextures;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * numOfInputAttachments * 2;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    //if (vkCreateDescriptorPool(_device._device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create descriptor pool!");
    //}

    descriptorPool = _device.createDescriptorPool( poolInfo );
}

void RenderEngine::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    depthTexture = new Texture( _device );

    depthTexture->createImage( _window.getExtent().width, _window.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat,
                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    //depthTexture->textureImageView = _device.createImageView(depthTexture->textureImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,1);
    depthTexture->createImageView( depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1 );

    //trasitionImageLayout(depthTexture->textureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,1);


    /*createImage(_swapChainExtent.width, _swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat);*/
}


void RenderEngine::createDescriptorSets()
{

    std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, descriptorSetLayout );
    descriptorSets = _device.createDescriptorSets( layouts, descriptorPool );

}

void RenderEngine::createDeferredDescriptorSets()
{

    std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, deferredDescriptorSetLayout );
    lightingDescriptorSets = _device.createDescriptorSets( layouts, descriptorPool );


}

void RenderEngine::updateGeometryDescriptorSets()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        int size = 1;

        if (_textureArray.size() > 0) {
            size++;
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites( size );

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i]->getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( UniformBufferObject );

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].dstSet = descriptorSets[i];



        std::vector<VkDescriptorImageInfo> imagesDesc;

        if (_textureArray.size() > 0) {

            for (auto& tex : _textureArray) {
                imagesDesc.push_back( tex->getTextureDescriptor() );
            }

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = imagesDesc.size();
            descriptorWrites[1].pImageInfo = imagesDesc.data();
            descriptorWrites[1].pBufferInfo = VK_NULL_HANDLE;
            descriptorWrites[1].dstSet = descriptorSets[i];
        }

        _device.updateDescriptorSet( descriptorWrites );
    }
}

void RenderEngine::updateLightingDescriptorSets()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        std::vector<VkWriteDescriptorSet> descriptorWrites( 6 );

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i]->getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( UniformBufferObject );

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].dstSet = lightingDescriptorSets[i];

        VkDescriptorImageInfo inputInfo{};
        inputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputInfo.imageView = colorTexture->textureImageView;
        inputInfo.sampler = VK_NULL_HANDLE;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstBinding = 2;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &inputInfo;
        descriptorWrites[1].dstSet = lightingDescriptorSets[i];


        VkDescriptorImageInfo inputInfo2{};
        inputInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputInfo2.imageView = normalTexture->textureImageView;
        inputInfo2.sampler = VK_NULL_HANDLE;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstBinding = 3;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &inputInfo2;
        descriptorWrites[2].dstSet = lightingDescriptorSets[i];


        VkDescriptorBufferInfo lightBuffer;
        lightBuffer.buffer = _lightUniformBuffer->getBuffer();
        lightBuffer.offset = 0;
        lightBuffer.range = sizeof( GlobalLighting );

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstBinding = 4;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &lightBuffer;
        descriptorWrites[3].dstSet = lightingDescriptorSets[i];

        //vkUpdateDescriptorSets(_device._device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);


        VkDescriptorImageInfo inputInfo3{};
        inputInfo3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputInfo3.imageView = posTexture->textureImageView;
        inputInfo3.sampler = VK_NULL_HANDLE;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstBinding = 5;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pImageInfo = &inputInfo3;
        descriptorWrites[4].dstSet = lightingDescriptorSets[i];



        VkDescriptorBufferInfo storageBufferInfo;
        storageBufferInfo.buffer = _lightBufferSorage->getBuffer();
        storageBufferInfo.offset = 0;
        storageBufferInfo.range = sizeof( int ) + (sizeof( Light ) * MAX_LIGHTS) + sizeof( uint8_t ) * 16;

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstBinding = 6;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pBufferInfo = &storageBufferInfo;
        descriptorWrites[5].dstSet = lightingDescriptorSets[i];


        //descriptorWritesVec.push_back( std::move( descriptorWrites ) );
        _device.updateDescriptorSet( descriptorWrites );
    }
}

void RenderEngine::pushModelMatrix( VkCommandBuffer commnadBuffer, glm::mat4 model )
{
    vkCmdPushConstants( commnadBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &model );
}

void RenderEngine::pushTextureIndex( VkCommandBuffer commnadBuffer, MaterialData material )
{
    vkCmdPushConstants( commnadBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof( glm::mat4 ), sizeof( material ), &material );
}


void RenderEngine::generateMipmaps( VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels )
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


RenderEngine::RenderEngine()
{
}

void RenderEngine::init()
{

    _window.init( "Proyecto Vulkan", WIDTH, HEIGHT, instance );
    createInstance();
    setupDebugMessenger();
    _window.createSurface( instance );
    _device.init( instance, _window );

    graphicsQueue = _device.getGraphicsQueue();
    presentQueue = _device.getPresentQueue();
    _window.setDevice( &_device );
    _window.createSwapChain();


    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDeferredPipeline();

    createColorResources();
    createDepthResources();
    createNormalResources();

    createFramebuffers();

    createCommandPool();
    createCommandBuffers();
    createSyncObjects();


    createDescriptorPool();

    //cracion de recursos

    createUniformBuffers();
    createLightBuffer();

    _mainCamera = Camera( glm::vec3( 0, 0, -2.5f ), glm::vec3( 0, 0, 1.f ), glm::vec3( 0.0f, 1.0f, 0.0f ),
                          90.f, _window.getExtent().width / (float)_window.getExtent().height, 0.1f, 40.f );


    createDescriptorSets();
    createDeferredDescriptorSets();
    updateGeometryDescriptorSets();
    updateLightingDescriptorSets();

}

void RenderEngine::createRenderPass()
{
    VkAttachmentDescription presentAttachment{};
    presentAttachment.format = _window.getFormat();
    presentAttachment.samples = VK_SAMPLE_COUNT_1_BIT /*_device.msaaSamples*/;
    presentAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    presentAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    presentAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    presentAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    presentAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    presentAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _window.getFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT /*_device.msaaSamples*/;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription normalAttachment{};
    normalAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT /*_device.msaaSamples*/;
    normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normalAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkAttachmentDescription posAttachment{};
    posAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    posAttachment.samples = VK_SAMPLE_COUNT_1_BIT /*_device.msaaSamples*/;
    posAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    posAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    posAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    posAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    posAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    posAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

    VkAttachmentReference presentAttachmentRef{}; //Salida de color
    presentAttachmentRef.attachment = 0;
    presentAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 2;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference normlaAttachmentRef{};
    normlaAttachmentRef.attachment = 3;
    normlaAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference posAttachmentRef{};
    posAttachmentRef.attachment = 4;
    posAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference inputReference{};
    inputReference.attachment = 2;
    inputReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inputReference2{};
    inputReference2.attachment = 3;
    inputReference2.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


    VkAttachmentReference inputReference3{};
    inputReference3.attachment = 4;
    inputReference3.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    //VkAttachmentReference colorAttachmentResolveRef{};
    //colorAttachmentResolveRef.attachment = 2;
    //colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference att[] = { colorAttachmentRef,normlaAttachmentRef,posAttachmentRef };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 3;
    subpass.pColorAttachments = att;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    //subpass.pResolveAttachments = &colorAttachmentResolveRef;


    VkAttachmentReference inputAttachments[] = { inputReference,inputReference2, inputReference3 };

    VkSubpassDescription lightingSubPass{};
    lightingSubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    lightingSubPass.colorAttachmentCount = 1;
    lightingSubPass.pColorAttachments = &presentAttachmentRef;
    lightingSubPass.pInputAttachments = inputAttachments;
    lightingSubPass.inputAttachmentCount = 3;

    /*
    Dependencias para permitir la transicion de los layouts
    */

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


    VkSubpassDependency dependency2{};
    dependency2.srcSubpass = 0;
    dependency2.dstSubpass = 1;
    dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency2.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    VkSubpassDependency dependency3{};
    dependency3.srcSubpass = 0;
    dependency3.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency3.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency3.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency3.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency3.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency3.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDescription subpasses[] = { subpass,lightingSubPass };

    VkSubpassDependency dependencies[] = { dependency,dependency2 , dependency3 };


    std::array<VkAttachmentDescription, 5> attachments = { presentAttachment, depthAttachment,colorAttachment, normalAttachment,posAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 2;
    renderPassInfo.pSubpasses = subpasses;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

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



void RenderEngine::populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo ) {

    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

}

bool RenderEngine::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp( layerName, layerProperties.layerName ) == 0) {
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