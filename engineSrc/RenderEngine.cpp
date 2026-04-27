#include "RenderEngine.h"
#include "Utils.h"
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

// ============================
// Lifecycle
// ============================

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
	_resources.releaseAllMaterials();
	_resources.releaseAllTextures();
	_resources.releaseAllMeshes();
	_resources.clear();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		delete uniformBuffers[i];
	}

	delete _lightUniformBuffer;
	delete _lightBufferStorage;
	delete _lightIndexStorage;
	delete _AABBModelStorage;
	delete _lightCulledObjectIndex;
	delete _cameraCulledObjectIndex;
	delete _mainLightVPBuffer;

	_device.destroyDescriptorPool( descriptorPool );

	_device.destroyDescriptorSetLayout( deferredDescriptorSetLayout );
	_device.destroyDescriptorSetLayout( _computeDescriptorSetLayout );
	_device.destroyDescriptorSetLayout( _inputAttachmentsDescriptorSetLayout );
	_device.destroyDescriptorSetLayout( _textureArrayDescriptorSetLayout );
	_device.destroyDescriptorSetLayout( _viewProjectionDescriptorSetLayout );
	_device.destroyDescriptorSetLayout( _indexedObjectsBufferDescriptroSetLayout );

	_device.destroyPipeline( graphicsPipeline );
	_device.destroyPipeline( _computePipeline );
	_device.destroyPipeline( deferredPipeline );
	_device.destroyPipeline( _shadowPipeline );
	_device.destroyPipeline( _objectCullingPipeline );


	_device.destroyPipelineLayout( pipelineLayout );
	_device.destroyPipelineLayout( deferredLayout );
	_device.destroyPipelineLayout( _computeLayout );
	_device.destroyPipelineLayout( _shadowPipelineLayout );
	_device.destroyPipelineLayout( _objectCullingPipelineLayout );


	//vkDestroyRenderPass(_device._device, renderPass, nullptr);
	_device.destroyRenderPass( renderPass );
	_device.destroyRenderPass( shadowPass );

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
	delete shadowMap;

	for (auto framebuffer : swapChainFramebuffers) {
		_device.destroyFrameBuffer( framebuffer );
	}
	_device.destroyFrameBuffer( _shadowFrameBuffer );

	_window.close();

	_device.close();

	//vkDestroyDevice(device, nullptr);

	if (enableValidationLayers) {

		DestroyDebugUtilsMessengerEXT( instance, debugMessenger, nullptr );
	}
	//glfwTerminate();
	vkDestroyInstance( instance, nullptr );

}

RenderEngine::RenderEngine() : _resources( _device )
{
	_resources.setTextureBindingsChangedCallback( [this]() {
		_device.wait();
		updateGeometryDescriptorSets();
	} );
}

void RenderEngine::init( const std::string& appName )
{

	_window.init( "appName", WIDTH, HEIGHT, instance );
	createInstance( appName );
	setupDebugMessenger();
	_window.createSurface( instance );
	_device.init( instance, _window );

	graphicsQueue = _device.getGraphicsQueue();
	presentQueue = _device.getPresentQueue();
	computeQueue = _device.getComputeQueue();
	_window.setDevice( &_device );
	_window.createSwapChain();

	createInputAttachmentDescriptorSetLayout();
	createTextureArrayDescriptorSetLayout();
	createViewProjectionDescriptorSetLayout();
	createIndexedObjectsBufferDescriptorSetLayout();


	createComputeDescriptorSetLayout();
	createComputePipeline();

	createObjectCullPipeline();

	createShadowPass();
	createShadowDescriptorSetLayout();
	createShadowPipeline();

	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createDeferredPipeline();

	createColorResources();
	createDepthResources();
	createNormalResources();
	createShadowResources();

	createFramebuffers();
	createShadowFrameBuffer();

	createCommandPool();
	createCommandBuffers();
	createSyncObjects();


	createDescriptorPool();

	//cracion de recursos

	createUniformBuffers();
	createLightBuffer();
	createCullingBuffers();

	_mainCamera = Camera( glm::vec3( 0, 0, -2.5f ), glm::vec3( 0, 0, 1.f ), glm::vec3( 0.0f, 1.0f, 0.0f ),
						  90.f, _window.getExtent().width / (float)_window.getExtent().height, 0.1f, 40.f );


	createGeometryDescriptorSets();
	createDeferredDescriptorSets();
	createComputeDescriptorSets();
	createShadowDescriptorSet();
	createObjectCullDescriptorSets();

	updateGeometryDescriptorSets();
	updateLightingDescriptorSets();
	updateComputeDescriptorSet();
	updateShadowDescriptorSet();
	updateCullDescriptorSet();

}

// ============================
// Vulkan bootstrap
// ============================

void RenderEngine::createInstance( const std::string& appName ) {



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
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName = "LL Engine";
	appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion = VK_API_VERSION_1_3;




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

namespace {
}

// ============================
// Pipeline creation
// ============================

void RenderEngine::createGraphicsPipeline()
{
	auto vertShaderCode = readFile( "./shaders/build/vertex" );
	auto fragShaderCode = readFile( "./shaders/build/fragment" );

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

	VkDescriptorSetLayout layouts[] = { _viewProjectionDescriptorSetLayout, _textureArrayDescriptorSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = 2;
	pipelineLayoutInfo.pPushConstantRanges = ranges;
	pipelineLayoutInfo.setLayoutCount = 2; // Optional
	pipelineLayoutInfo.pSetLayouts = layouts; // Optional

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


	VkDescriptorSetLayout layouts[] = {
		_inputAttachmentsDescriptorSetLayout,
		_viewProjectionDescriptorSetLayout,
		_indexedObjectsBufferDescriptroSetLayout ,
		deferredDescriptorSetLayout
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 4; // Optional
	pipelineLayoutInfo.pSetLayouts = layouts; // Optional

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


	deferredPipeline = _device.createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

	_device.destroyShaderModule( vertShaderModule );
	_device.destroyShaderModule( fragShaderModule );
}

void RenderEngine::createComputePipeline()
{

	auto computeCode = readFile( "./shaders/build/lightCull" );

	VkShaderModule computeModule = _device.createShaderModule( computeCode );

	VkPipelineShaderStageCreateInfo computeStageInfo{};
	computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeStageInfo.module = computeModule;
	computeStageInfo.pName = "main";

	VkDescriptorSetLayout layouts[]{ _indexedObjectsBufferDescriptroSetLayout, _computeDescriptorSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2; // Optional
	pipelineLayoutInfo.pSetLayouts = layouts; // Optional

	_computeLayout = _device.createPipelineLayout( pipelineLayoutInfo );

	VkComputePipelineCreateInfo createInfo;

	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.layout = _computeLayout;
	createInfo.stage = computeStageInfo;
	createInfo.flags = 0;
	createInfo.pNext = NULL;

	_computePipeline = _device.createcomputePipelines( VK_NULL_HANDLE, {createInfo} )[0];

	_device.destroyShaderModule( computeModule );
}

void RenderEngine::createObjectCullPipeline()
{
	auto computeCode = readFile( "./shaders/build/objectCull" );

	VkShaderModule computeModule = _device.createShaderModule( computeCode );

	VkPipelineShaderStageCreateInfo computeStageInfo{};
	computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeStageInfo.module = computeModule;
	computeStageInfo.pName = "main";

	VkDescriptorSetLayout layouts[]{ _indexedObjectsBufferDescriptroSetLayout, _viewProjectionDescriptorSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2; // Optional
	pipelineLayoutInfo.pSetLayouts = layouts; // Optional

	_objectCullingPipelineLayout = _device.createPipelineLayout( pipelineLayoutInfo );

	VkComputePipelineCreateInfo createInfo;

	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.layout = _objectCullingPipelineLayout;
	createInfo.stage = computeStageInfo;
	createInfo.flags = 0;
	createInfo.pNext = NULL;

	_objectCullingPipeline = _device.createcomputePipelines( VK_NULL_HANDLE, { createInfo } )[0];

	_device.destroyShaderModule( computeModule );
}

void RenderEngine::createShadowPipeline()
{
	auto vertShaderCode = readFile( "./shaders/build/shadowVertex" );

	VkShaderModule vertShaderModule = _device.createShaderModule( vertShaderCode );



	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";


	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo };


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

		VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


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


	VkPushConstantRange range{};
	range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	range.size = sizeof( glm::mat4 );
	range.offset = 0;

	VkPushConstantRange ranges[] = { range };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = ranges;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &_viewProjectionDescriptorSetLayout; // Optional

	_shadowPipelineLayout = _device.createPipelineLayout( pipelineLayoutInfo );

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 1;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; // Optional
	pipelineInfo.pColorBlendState = nullptr;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = _shadowPipelineLayout;
	pipelineInfo.renderPass = shadowPass;
	pipelineInfo.subpass = 0;


	_shadowPipeline = _device.createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

	_device.destroyShaderModule( vertShaderModule );
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

void RenderEngine::createShadowFrameBuffer()
{
	//el orden debe coincidir con la especificacion realizada en la descripcion de la pipeline
	std::array<VkImageView, 1> attachments = {
		shadowMap->textureImageView
	};

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = shadowPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = shadowMap->texWidth;
	framebufferInfo.height = shadowMap->texHeight;
	framebufferInfo.layers = 1;

	//if (vkCreateFramebuffer(_device._device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
	//    throw std::runtime_error("failed to create framebuffer!");
	//}

	_shadowFrameBuffer = _device.createFrameBuffer( framebufferInfo );
}

void RenderEngine::createCommandPool()
{
	VulkanDevice::QueueFamilyIndices queueFamilyIndices = _device.getFamilyIndexes();

	commandPool = _device.createCommandPool( VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndices.presentFamily.value());
	//transferPool = _device.createCommandPool( VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndices.transferFamily.value() );
}

void RenderEngine::createCommandBuffers()
{

	commandBuffers = _device.createCommandBuffers( commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT );

}

// ============================
// Frame recording and presentation
// ============================

void RenderEngine::recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<RenderObject>& objectsArray, const std::vector<int>&cullIndex )
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


	//std::vector<AABBModel> stagingCull;
	//stagingCull.reserve( objectsArray.size() );

	//for (auto object : objectsArray) {
	//	AABBModel amod;
	//	amod.maxBound = object.mesh->getAABB().max;
	//	amod.minBound = object.mesh->getAABB().min;
	//	amod.modelMat = object.modelMatrix;
	//	stagingCull.emplace_back( amod );
	//}

	//int size = stagingCull.size();

	//memcpy( _AABBModelStorageMapped, &size, sizeof( int ) );

	////+16 por alineamietno en memoria en la gpu
	//void* arrayStart = (uint8_t*)_AABBModelStorageMapped + 16;
	//memcpy( arrayStart, stagingCull.data(), sizeof( AABBModel ) * stagingCull.size() );


	//vkCmdBindPipeline( commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, _objectCullingPipeline );

	//VkDescriptorSet cullSets[] = { _cameraCullDescriptorSet, _cameraDescriptorSet[currentFrame] };

	//vkCmdBindDescriptorSets( commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, _objectCullingPipelineLayout, 0, 2, cullSets, 0, nullptr );

	//vkCmdDispatch( commandBuffers[currentFrame], (objectsArray.size() / 32) + 1, 1, 1 );


	//VkDescriptorSet cullSets2[] = { _lightCullDescriptorSet, _mainLightDescriptorSet };

	//vkCmdBindDescriptorSets( commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, _objectCullingPipelineLayout, 0, 2, cullSets2, 0, nullptr );

	//vkCmdDispatch( commandBuffers[currentFrame], (objectsArray.size() / 32) + 1, 1, 1 );


	VkBufferMemoryBarrier memBarr;
	memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	memBarr.buffer = _lightIndexStorage->getBuffer();
	memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	memBarr.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	memBarr.offset = 0;
	memBarr.size = sizeof( int ) + (sizeof( int ) * MAX_LIGHTS);
	memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarr.pNext = NULL;

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipeline );

	VkDescriptorSet computeSets[] = {_lightsDataBufferDescriptroSet,_computeDescriptorSet[currentFrame] };

	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computeLayout, 0, 2, computeSets, 0, nullptr);

	vkCmdDispatch( commandBuffer, (MAX_LIGHTS/32)+1, 1, 1 );

	vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &memBarr, 0, nullptr );

	recordShadowPass( commandBuffer, objectsArray );

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
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

	VkDescriptorSet descriptors[] = { _cameraDescriptorSet[currentFrame],_textureArrayDescriptorSet };

	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, descriptors, 0, nullptr );
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



	for (int index : cullIndex) {
		const RenderObject& object = objectsArray[index];
		const Mesh* mesh = _resources.tryGetMesh( object.mesh );
		if (mesh == nullptr) {
			continue;
		}

		const MaterialData defaultMaterial{};
		const MaterialData* material = _resources.tryGetMaterial( object.material );
		if (material == nullptr) {
			material = &defaultMaterial;
		}

		pushTextureIndex( commandBuffer, *material );
		pushModelMatrix( commandBuffer, object.modelMatrix );

		mesh->draw( commandBuffer );
	}

	vkCmdNextSubpass( commandBuffer, VK_SUBPASS_CONTENTS_INLINE );

	VkDescriptorSet sets[] = {
		_inputAttachemntsDescriptorSet[currentFrame],
		_mainLightDescriptorSet,
		_lightsDataBufferDescriptroSet,
		_globalLightingDescriptorSets[currentFrame]
	};

	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLayout, 0, 4, sets, 0, nullptr );

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline );

	//Si no se han renderizado objetos no se debe llamar al dibujado de deferred puede dar a problemass
	if (cullIndex.size() > 0) {
		vkCmdDraw( commandBuffer, 3, 1, 0, 0 );
	}

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
void RenderEngine::drawFrame()
{
	if (_framebufferResized) {
		recreateSwapChain();
		updateGeometryDescriptorSets();
		updateLightingDescriptorSets();
		_framebufferResized = false;
		return;
	}

	const std::vector<RenderObject>& objectsArray = _scene.buildRenderQueue();
	const std::vector<LightObject>& lightQueue = _scene.buildLightQueue();

	_device.waitForFences(inFlightFences[currentFrame]);
	uint32_t imageIndex;
	VkResult result = _device.adquireNextImage(
		_window.getSwapChain(), imageAviablesSemaphores[currentFrame], imageIndex);


	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		updateGeometryDescriptorSets();
		updateLightingDescriptorSets();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error( "failed to acquire swap chain image!" );
	}

	ViewProjectionData camDesc;
	camDesc.proj = _mainCamera.getProjMatrix();
	camDesc.view = _mainCamera.getViewMatrix();
	auto outObjects = cullObjects( objectsArray, camDesc);

	updateUniformBuffer( currentFrame, glm::mat4( 1.0f ) );
	uploadLightBuffer(lightQueue);
	_device.resetFences( inFlightFences[currentFrame] );


	if (outObjects.size() == 0) {
		std::cout << "No hay objetos a pintar\n";
	}

	Mesh::_lastRenderedMesh = nullptr;
	vkResetCommandBuffer( commandBuffers[currentFrame], 0 );
	recordCommandBuffer( commandBuffers[currentFrame], imageIndex, objectsArray, outObjects );

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAviablesSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
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
	_mainCamera.setAspectRatio( _window.getExtent().width / static_cast<float>( _window.getExtent().height ) );
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

void RenderEngine::updateUniformBuffer(uint32_t currentImage, glm::mat4 model) {
	ViewProjectionData ubo{};
	ubo.view = _mainCamera.getViewMatrix();
	ubo.proj = _mainCamera.getProjMatrix();
	ubo.proj[1][1] *= -1;
	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

	_lighting.eyePos = _mainCamera.getPos();
	memcpy(_lightBufferMapped, &_lighting, sizeof(_lighting));

	// Shadow pass VP: se construye a partir de la main light de la escena.
	// Si no hay main light designada o fue destruida, dejamos el UBO como esta;
	// recordShadowPass() comprobara lo mismo y omitira el pase si es nullptr.
	float ratio = _window.getExtent().width / static_cast<float>(_window.getExtent().height);
	float scale = 20.0f;
	_mainLightVPMapped->proj = glm::ortho(-ratio * scale, ratio * scale,
		scale, -scale, 0.01f, 100.0f);

	const LightObject* mainLight = _scene.tryGetMainLight();
	glm::vec3 lightDir(0.0f, -1.0f, 0.0f);
	if (mainLight != nullptr) {
		lightDir = mainLight->posOrDir;
	}
	glm::vec3 position = glm::vec3(5.0f, 10.0f, -10.0f) + _mainCamera.getPos();
	_mainLightVPMapped->view = glm::lookAt(position, position + lightDir, glm::vec3(0, 1, 0));
}

void RenderEngine::updateComputeDescriptorSet()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		int size = 3;

		std::vector<VkWriteDescriptorSet> descriptorWrites( size );

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _lightIndexStorage->getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof( int ) + (sizeof( int ) * MAX_LIGHTS);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].dstSet = _lightsDataBufferDescriptroSet;

		VkDescriptorBufferInfo storageBufferInfo2;
		storageBufferInfo2.buffer = _lightBufferStorage->getBuffer();
		storageBufferInfo2.offset = 0;
		storageBufferInfo2.range = sizeof( int ) + (sizeof(LightObject) * MAX_LIGHTS) + sizeof( uint8_t ) * 16;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &storageBufferInfo2;
		descriptorWrites[1].dstSet = _lightsDataBufferDescriptroSet;

		VkDescriptorBufferInfo lightBuffer;
		lightBuffer.buffer = _lightUniformBuffer->getBuffer();
		lightBuffer.offset = 0;
		lightBuffer.range = sizeof( GlobalLighting );

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &lightBuffer;
		descriptorWrites[2].dstSet = _computeDescriptorSet[i];


		_device.updateDescriptorSet( descriptorWrites );
	}
}

void RenderEngine::updateShadowDescriptorSet()
{
	int size = 1;

	std::vector<VkWriteDescriptorSet> descriptorWrites( size );

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = _mainLightVPBuffer->getBuffer();
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof( ViewProjectionData );

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;
	descriptorWrites[0].dstSet = _mainLightDescriptorSet;

	_device.updateDescriptorSet( descriptorWrites );
}

void RenderEngine::updateCullDescriptorSet()
{
	int size = 4;

	std::vector<VkWriteDescriptorSet> descriptorWrites( size );

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = _cameraCulledObjectIndex->getBuffer();
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;
	descriptorWrites[0].dstSet = _cameraCullDescriptorSet;

	VkDescriptorBufferInfo bufferInfo2{};
	bufferInfo2.buffer = _AABBModelStorage->getBuffer();
	bufferInfo2.offset = 0;
	bufferInfo2.range = VK_WHOLE_SIZE;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo2;
	descriptorWrites[1].dstSet = _cameraCullDescriptorSet;


	VkDescriptorBufferInfo bufferInfo3{};
	bufferInfo3.buffer = _lightCulledObjectIndex->getBuffer();
	bufferInfo3.offset = 0;
	bufferInfo3.range = VK_WHOLE_SIZE;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstBinding = 0;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &bufferInfo3;
	descriptorWrites[2].dstSet = _lightCullDescriptorSet;

	VkDescriptorBufferInfo bufferInfo4{};
	bufferInfo4.buffer = _AABBModelStorage->getBuffer();
	bufferInfo4.offset = 0;
	bufferInfo4.range = VK_WHOLE_SIZE;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstBinding = 1;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &bufferInfo4;
	descriptorWrites[3].dstSet = _lightCullDescriptorSet;

	_device.updateDescriptorSet( descriptorWrites );
}


void RenderEngine::createLightBuffer()
{
	_lighting.eyePos = glm::vec3( 0, 0, -2.5f );

	_lighting.ambientVal = 0.01f;

	VkDeviceSize memorySize = sizeof( int ) + (sizeof( LightObject ) * MAX_LIGHTS) + sizeof( uint8_t ) * 16;
	_lightBufferStorage = _device.createBuffer( memorySize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	_device.mapMemory( _lightBufferStorage->getMemory(), 0, memorySize, &_lightBufferStorageMapped );

	//BUffer de indicies (el plan es que solo exista en gpu ya que es para hacer el culling de luces en un shader de computo)
	memorySize = sizeof( int ) + (sizeof( int ) * MAX_LIGHTS);
	_lightIndexStorage = _device.createBuffer( memorySize,(VkBufferUsageFlagBits)(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	//memorySize = sizeof( int ) + (sizeof( AABBModel ) * MAX_CULL_OBJECTS);
	//_AABBModelStorage = _device.createBuffer( memorySize, (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	//_device.mapMemory( _AABBModelStorage->getMemory(), 0, memorySize, &_AABBModelStorageMapped);


	//std::vector<uint32_t> index;
	//for (int i = 0; i < MAX_LIGHTS;i++) {
	//	index.push_back( i );
	//}

	//Buffer* staginBuffer = _device.createBuffer( memorySize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	//									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	//
	//int size = index.size();
	//
	//void* dataMap;
	//_device.mapMemory( staginBuffer->getMemory(), 0, sizeof( uint32_t ), &dataMap);
	//memcpy( dataMap, &size, sizeof( uint32_t ) );
	//_device.unmapMemory( staginBuffer->getMemory() );
	//_device.mapMemory( staginBuffer->getMemory(), sizeof( uint32_t ), memorySize, &dataMap);
	//memcpy( dataMap, index.data(), index.size() * sizeof( uint32_t ) );
	//_device.unmapMemory( staginBuffer->getMemory() );

	//_device.copyBuffer( staginBuffer->getBuffer(), _lightIndexStorage->getBuffer(), memorySize);

	//delete staginBuffer;

}

void RenderEngine::createCullingBuffers()
{
	VkDeviceSize memorySize = sizeof( int ) + (sizeof( AABBModel ) * MAX_CULL_OBJECTS) + sizeof( uint8_t ) * 16;
	_AABBModelStorage = _device.createBuffer( memorySize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	_device.mapMemory( _AABBModelStorage->getMemory(), 0, memorySize, &_AABBModelStorageMapped );

	memorySize= sizeof( int ) + (sizeof( int ) * MAX_CULL_OBJECTS);
	_lightCulledObjectIndex = _device.createBuffer( memorySize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	//_device.mapMemory( _lightCulledObjectIndex->getMemory(), 0, sizeof( int ),(void**)&_lightObjectCount);
	_device.mapMemory( _lightCulledObjectIndex->getMemory(), 0, memorySize,(void**)&_lightCulledObjectIndexMapped);

	_cameraCulledObjectIndex = _device.createBuffer( memorySize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	//_device.mapMemory( _cameraCulledObjectIndex->getMemory(), 0, sizeof( int ), (void**)&_cameraObjectCount );
	_device.mapMemory( _cameraCulledObjectIndex->getMemory(), 0, memorySize, (void**)&_cameraCulledObjectIndexMapped );
}

void RenderEngine::uploadLightBuffer(const std::vector<LightObject>& lights) {
	if (lights.size() >= MAX_LIGHTS) {
		throw std::runtime_error("RenderEngine::uploadLightBuffer exceeded MAX_LIGHTS");
	}

	const int size = static_cast<int>(lights.size());
	memcpy(_lightBufferStorageMapped, &size, sizeof(int));

	// +16 por alineamiento en memoria de la GPU.
	void* arrayStart = static_cast<uint8_t*>(_lightBufferStorageMapped) + 16;
	memcpy(arrayStart, lights.data(), sizeof(LightObject) * lights.size());
}

void RenderEngine::handleWindowEvent( const WindowEvent& event )
{
	if (event.type == WindowEventType::Resized) {
		_framebufferResized = true;
	}
	_window.handleWindowEvent( event );
}

void RenderEngine::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding lightingBinding{};
	lightingBinding.binding = 3;
	lightingBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightingBinding.descriptorCount = 1;
	lightingBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	lightingBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutBinding shadowMap{};
	shadowMap.binding = 6;
	shadowMap.descriptorCount = 1;
	shadowMap.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadowMap.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	shadowMap.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings2 = { /*input1, input2 ,input3,*/ lightingBinding,shadowMap };
	VkDescriptorSetLayoutCreateInfo deferredLayout{};
	deferredLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	deferredLayout.bindingCount = static_cast<uint32_t>(bindings2.size());
	deferredLayout.pBindings = bindings2.data();

	deferredDescriptorSetLayout = _device.createDescriptorSetLayout( deferredLayout );

}

void RenderEngine::createComputeDescriptorSetLayout()
{


	VkDescriptorSetLayoutBinding lightingBinding{};
	lightingBinding.binding = 2;
	lightingBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightingBinding.descriptorCount = 1;
	lightingBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	lightingBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { lightingBinding };

	VkDescriptorSetLayoutCreateInfo computeLayout{};
	computeLayout.bindingCount = static_cast<uint32_t>(bindings.size());
	computeLayout.pBindings = bindings.data();
	computeLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	_computeDescriptorSetLayout = _device.createDescriptorSetLayout(computeLayout);
}

void RenderEngine::createShadowDescriptorSetLayout()
{
	//VkDescriptorSetLayoutBinding uboLayoutBinding{};
	//uboLayoutBinding.binding = 0;
	//uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//uboLayoutBinding.descriptorCount = 1;
	//uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//uboLayoutBinding.pImmutableSamplers = nullptr;


	//std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };
	//VkDescriptorSetLayoutCreateInfo layoutInfo{};
	//layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	//layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	//layoutInfo.pBindings = bindings.data();
	//layoutInfo.pNext = NULL;


	//_shadowDescriptorSetLayout = _device.createDescriptorSetLayout( layoutInfo );
}

void RenderEngine::createInputAttachmentDescriptorSetLayout()
{

	//color
	VkDescriptorSetLayoutBinding input1{};
	input1.binding = 0;
	input1.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	input1.descriptorCount = 1;
	input1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//normal
	VkDescriptorSetLayoutBinding input2{};
	input2.binding = 1;
	input2.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	input2.descriptorCount = 1;
	input2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	//position
	VkDescriptorSetLayoutBinding input3{};
	input3.binding = 2;
	input3.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	input3.descriptorCount = 1;
	input3.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { input1,input2, input3 };

	VkDescriptorSetLayoutCreateInfo inputLayout{};
	inputLayout.bindingCount = static_cast<uint32_t>(bindings.size());
	inputLayout.pBindings = bindings.data();
	inputLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	_inputAttachmentsDescriptorSetLayout = _device.createDescriptorSetLayout( inputLayout );
}

void RenderEngine::createTextureArrayDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = MAX_TEXTURES;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//flags para determinar que se trata de un binding de ocupacion variable
	VkDescriptorBindingFlags flags[1];
	flags[0] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags{};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = 1;
	binding_flags.pBindingFlags = flags;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &samplerLayoutBinding;
	layoutInfo.pNext = &binding_flags;

	_textureArrayDescriptorSetLayout = _device.createDescriptorSetLayout( layoutInfo );

}

void RenderEngine::createViewProjectionDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;


	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	layoutInfo.pNext = nullptr;

	_viewProjectionDescriptorSetLayout = _device.createDescriptorSetLayout( layoutInfo );
}

void RenderEngine::createIndexedObjectsBufferDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding indexBuffer{};
	indexBuffer.binding = 0;
	indexBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	indexBuffer.descriptorCount = 1;
	indexBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	indexBuffer.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding objectBuffer{};
	objectBuffer.binding = 1;
	objectBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	objectBuffer.descriptorCount = 1;
	objectBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT| VK_SHADER_STAGE_FRAGMENT_BIT;
	objectBuffer.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { indexBuffer,objectBuffer };

	VkDescriptorSetLayoutCreateInfo computeLayout{};
	computeLayout.bindingCount = static_cast<uint32_t>(bindings.size());
	computeLayout.pBindings = bindings.data();
	computeLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	_indexedObjectsBufferDescriptroSetLayout = _device.createDescriptorSetLayout( computeLayout );
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
	VkDeviceSize bufferSize = sizeof( ViewProjectionData );

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


	// Shadow pass VP buffer
	VkDeviceSize mainLightBufferSize = sizeof( ViewProjectionData );
	_mainLightVPBuffer = _device.createBuffer( 
		mainLightBufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	_device.mapMemory( _mainLightVPBuffer->getMemory(), 0, mainLightBufferSize,
					   reinterpret_cast<void**>( &_mainLightVPMapped ) );
}

void RenderEngine::createShadowResources()
{
	VkFormat depthFormat = findDepthFormat();
	shadowMap = new Texture(_device);

	shadowMap->createImage(1920, 1080, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	shadowMap->createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	shadowMap->createTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
}

void RenderEngine::createDescriptorPool()
{
	int numOfInputAttachments = 3;

	int numOfUniformBuffres = 4;

	int numOfStorageBuffers = 6;

	std::array<VkDescriptorPoolSize, 4> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * numOfUniformBuffres+1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_TEXTURES) + static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * numOfInputAttachments * 2;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)*numOfStorageBuffers;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 5+5;

	descriptorPool = _device.createDescriptorPool( poolInfo );
}

void RenderEngine::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat();

	depthTexture = new Texture( _device );

	depthTexture->createImage( _window.getExtent().width, _window.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat,
							   VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	depthTexture->createImageView( depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1 );

}


void RenderEngine::createGeometryDescriptorSets()
{

	std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT,_viewProjectionDescriptorSetLayout );


	_cameraDescriptorSet = _device.createDescriptorSets( layouts, descriptorPool);


	//adicion que permite tener arrays de tamanio variable en los shaders de texturas
	std::vector<VkDescriptorSetLayout> layouts2( 1, _textureArrayDescriptorSetLayout );
	uint32_t counts = static_cast<uint32_t>(MAX_TEXTURES);
	VkDescriptorSetVariableDescriptorCountAllocateInfo set_counts = {};
	set_counts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	set_counts.descriptorSetCount = 1;
	set_counts.pDescriptorCounts = &counts;

	_textureArrayDescriptorSet = _device.createDescriptorSets( layouts2, descriptorPool,&set_counts )[0];

}

void RenderEngine::createDeferredDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, deferredDescriptorSetLayout );
	_globalLightingDescriptorSets = _device.createDescriptorSets( layouts, descriptorPool );

	std::vector<VkDescriptorSetLayout> layouts2( MAX_FRAMES_IN_FLIGHT, _inputAttachmentsDescriptorSetLayout );
	_inputAttachemntsDescriptorSet = _device.createDescriptorSets( layouts2, descriptorPool );

	std::vector<VkDescriptorSetLayout> layouts3( 1, _viewProjectionDescriptorSetLayout);
	_mainLightDescriptorSet = _device.createDescriptorSets( layouts3, descriptorPool )[0];

	std::vector<VkDescriptorSetLayout> layouts4( 1, _indexedObjectsBufferDescriptroSetLayout );
	_lightsDataBufferDescriptroSet = _device.createDescriptorSets( layouts4, descriptorPool )[0];
}

void RenderEngine::createComputeDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts2( MAX_FRAMES_IN_FLIGHT, _computeDescriptorSetLayout );
	_computeDescriptorSet = _device.createDescriptorSets( layouts2, descriptorPool );
}

void RenderEngine::createObjectCullDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts2( MAX_FRAMES_IN_FLIGHT, _indexedObjectsBufferDescriptroSetLayout );
	_cameraCullDescriptorSet = _device.createDescriptorSets( layouts2, descriptorPool )[0];
	_lightCullDescriptorSet = _device.createDescriptorSets( layouts2, descriptorPool )[0];
}

void RenderEngine::createShadowDescriptorSet()
{

}

void RenderEngine::updateGeometryDescriptorSets()
{
	const std::vector<ResourceManager::TextureBindingEntry> textureEntries = _resources.getLiveTextureEntries();

	if (textureEntries.size() > static_cast<size_t>(MAX_TEXTURES)) {
		throw std::runtime_error( "RenderEngine::updateGeometryDescriptorSets exceeded MAX_TEXTURES" );
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		std::vector<VkWriteDescriptorSet> descriptorWrites( 1 + textureEntries.size() );

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i]->getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof( ViewProjectionData );

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].dstSet = _cameraDescriptorSet[i];

		std::vector<VkDescriptorImageInfo> imagesDesc;
		imagesDesc.reserve( textureEntries.size() );

		for (size_t textureWriteIndex = 0; textureWriteIndex < textureEntries.size(); ++textureWriteIndex) {
			imagesDesc.push_back( textureEntries[textureWriteIndex].texture->getTextureDescriptor() );

			VkWriteDescriptorSet& write = descriptorWrites[textureWriteIndex + 1];
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstBinding = 0;
			write.dstArrayElement = textureEntries[textureWriteIndex].index;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.descriptorCount = 1;
			write.pImageInfo = &imagesDesc[textureWriteIndex];
			write.pBufferInfo = VK_NULL_HANDLE;
			write.dstSet = _textureArrayDescriptorSet;
		}

		_device.updateDescriptorSet( descriptorWrites );
	}
}

void RenderEngine::updateLightingDescriptorSets()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		std::vector<VkWriteDescriptorSet> descriptorWrites( 6 );

		VkDescriptorImageInfo inputInfo{};
		inputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		inputInfo.imageView = colorTexture->textureImageView;
		inputInfo.sampler = VK_NULL_HANDLE;

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &inputInfo;
		descriptorWrites[0].dstSet = _inputAttachemntsDescriptorSet[i];


		VkDescriptorImageInfo inputInfo2{};
		inputInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		inputInfo2.imageView = normalTexture->textureImageView;
		inputInfo2.sampler = VK_NULL_HANDLE;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &inputInfo2;
		descriptorWrites[1].dstSet = _inputAttachemntsDescriptorSet[i];


		VkDescriptorImageInfo inputInfo3{};
		inputInfo3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		inputInfo3.imageView = posTexture->textureImageView;
		inputInfo3.sampler = VK_NULL_HANDLE;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstBinding = 2;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &inputInfo3;
		descriptorWrites[3].dstSet = _inputAttachemntsDescriptorSet[i];


		VkDescriptorBufferInfo lightBuffer;
		lightBuffer.buffer = _lightUniformBuffer->getBuffer();
		lightBuffer.offset = 0;
		lightBuffer.range = sizeof( GlobalLighting );

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstBinding = 3;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &lightBuffer;
		descriptorWrites[2].dstSet = _globalLightingDescriptorSets[i];


		VkDescriptorImageInfo imgInfo = shadowMap->getTextureDescriptor( VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL );

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstBinding = 6;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pImageInfo = &imgInfo;
		descriptorWrites[4].pBufferInfo = VK_NULL_HANDLE;
		descriptorWrites[4].dstSet = _globalLightingDescriptorSets[i];

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _mainLightVPBuffer->getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof( ViewProjectionData );

		descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[5].dstBinding = 0;
		descriptorWrites[5].dstArrayElement = 0;
		descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[5].descriptorCount = 1;
		descriptorWrites[5].pBufferInfo = &bufferInfo;
		descriptorWrites[5].dstSet = _mainLightDescriptorSet;


		//descriptorWritesVec.push_back( std::move( descriptorWrites ) );
		_device.updateDescriptorSet( descriptorWrites );
	}
}

void RenderEngine::pushModelMatrix( VkCommandBuffer commnadBuffer, glm::mat4 model )
{
	vkCmdPushConstants( commnadBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &model );
}

void RenderEngine::pushTextureIndex( VkCommandBuffer commnadBuffer, const MaterialData& material )
{
	vkCmdPushConstants( commnadBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof( glm::mat4 ), sizeof( material ), &material );
}

void RenderEngine::recordShadowPass( VkCommandBuffer commandBuffer, const std::vector<RenderObject>& objectsArray )
{
	// Si no hay main light designada o fue destruida, no hay shadow pass.
	if (!_scene.hasMainLight()) {
		return;
	}

	std::vector<int> cullIndex = cullObjects(objectsArray, *_mainLightVPMapped);

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.framebuffer = _shadowFrameBuffer;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = shadowPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	VkExtent2D ext{ shadowMap->texWidth, shadowMap->texHeight };
	renderPassInfo.renderArea.extent = ext;

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	//tomamos los datos de paso de renderizado
	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipelineLayout, 0, 1, &_mainLightDescriptorSet, 0, nullptr );
	//Aplicamos la pipeline definida
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipeline );

	//Llamadas a los estados dinamicos
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>( shadowMap->texWidth );
	viewport.height = static_cast<float>( shadowMap->texHeight );
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport( commandBuffer, 0, 1, &viewport );

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = ext;
	vkCmdSetScissor( commandBuffer, 0, 1, &scissor );

	for (auto& index : cullIndex) {
		const Mesh* mesh = _resources.tryGetMesh( objectsArray[index].mesh );
		if (mesh == nullptr) {
			continue;
		}
		pushModelMatrix( commandBuffer, objectsArray[index].modelMatrix);
		mesh->draw( commandBuffer );
	}

	vkCmdEndRenderPass( commandBuffer );

	// Transicion del shadow map para lectura en el fragment shader.
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = shadowMap->textureImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier( 
		commandBuffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier );
}

const std::vector<int> RenderEngine::cullObjects( const std::vector<RenderObject>& objs, ViewProjectionData& cameraDesc )
{
	auto start = std::chrono::high_resolution_clock::now();
	std::vector<int> out;
	out.reserve(objs.size());
	glm::mat4 VP = cameraDesc.proj * cameraDesc.view;

	int i = 0;
	for (const auto& obj : objs) {
		const Mesh* mesh = _resources.tryGetMesh( obj.mesh );
		if (mesh == nullptr) {
			i++;
			continue;
		}

		glm::mat4 MVP = VP * obj.modelMatrix;
		if (AABBFrustrumTest( mesh->getAABB(),MVP )) {
			out.emplace_back(i);
		}
		i++;
	}
	auto end = std::chrono::high_resolution_clock::now();

	//std::cout <<"Objects To paint: "<< out.size() << " Time to process object Culling: " << std::chrono::duration<float, std::chrono::milliseconds::period>(end-start) << "\n";
	return out;
}

const std::vector<LightObject> RenderEngine::cullLights( const std::vector<LightObject>& lights )
{
	std::vector<LightObject> out;
	glm::mat4 VP = _mainCamera.getProjMatrix() * _mainCamera.getViewMatrix();


	for (auto& light : lights) {

		glm::vec4 pos = VP*glm::vec4(light.posOrDir,1);

		if ((pos.w <= _mainCamera.getFarPlane()+100 && pos.w >= -100) || light.type == LightType::Directional) {
			out.push_back( light );
		}
	}

	return out;
}


bool RenderEngine::AABBFrustrumTest( const AABB& aabb, const glm::mat4& MVP )
{
	//auto start = std::chrono::high_resolution_clock::now();
	glm::vec4 corners[8] = {
		{aabb.min.x, aabb.min.y, aabb.min.z, 1.0}, // x y z
		{aabb.max.x, aabb.min.y, aabb.min.z, 1.0}, // X y z
		{aabb.min.x, aabb.max.y, aabb.min.z, 1.0}, // x Y z
		{aabb.max.x, aabb.max.y, aabb.min.z, 1.0}, // X Y z

		{aabb.min.x, aabb.min.y, aabb.max.z, 1.0}, // x y Z
		{aabb.max.x, aabb.min.y, aabb.max.z, 1.0}, // X y Z
		{aabb.min.x, aabb.max.y, aabb.max.z, 1.0}, // x Y Z
		{aabb.max.x, aabb.max.y, aabb.max.z, 1.0}, // X Y Z
	};

	bool inside = false;

	for (size_t corner_idx = 0; corner_idx < 8 && !inside; corner_idx++) {
		// Transform vertex
		glm::vec4 corner = MVP * corners[corner_idx];
		// Check vertex against clip space bounds
		inside = inside || (
			(corner.x <= corner.w && corner.x > -corner.w)&&
			(corner.y <= corner.w && corner.y > -corner.w)&&
			(corner.z <= corner.w && corner.w > 0)
			);
	}

	/*auto end = std::chrono::high_resolution_clock::now();

	std::cout << " Time to process AABB: " << std::chrono::duration<float, std::chrono::milliseconds::period>( end - start ) << "\n";*/

	return inside;
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
	colorAttachment.samples = _msaaSamples /*_device.msaaSamples*/;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription normalAttachment{};
	normalAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	normalAttachment.samples = _msaaSamples /*_device.msaaSamples*/;
	normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	VkAttachmentDescription posAttachment{};
	posAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	posAttachment.samples = _msaaSamples /*_device.msaaSamples*/;
	posAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	posAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	posAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	posAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	posAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	posAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = _msaaSamples;
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

	VkSubpassDependency dependency4{};
	dependency4.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency4.dstSubpass = 1;
	dependency4.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	dependency4.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency4.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	dependency4.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependency4.dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT;

	VkSubpassDescription subpasses[] = { subpass,lightingSubPass };

	VkSubpassDependency dependencies[] = { dependency,dependency2 , dependency3, dependency4 };

	std::array<VkAttachmentDescription, 5> attachments = { presentAttachment, depthAttachment,colorAttachment, normalAttachment,posAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 2;
	renderPassInfo.pSubpasses = subpasses;
	renderPassInfo.dependencyCount = 4;
	renderPassInfo.pDependencies = dependencies;

	renderPass = _device.createRenderPass( renderPassInfo );
}

void RenderEngine::createShadowPass()
{
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pColorAttachments = NULL;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkAttachmentDescription, 1> attachments = { depthAttachment};
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;


	shadowPass = _device.createRenderPass( renderPassInfo );
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
