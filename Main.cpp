#define VK_USE_PLATFORM_WIN32_KHR

#define SDL_MAIN_HANDLED

#define MAX_FRAMES_IN_FLIGHT 2

#include<iostream>

#include<algorithm>
#include<glm/glm.hpp>
#include<SDL2/SDL.h>
#include<SDL2/SDL_syswm.h>
#include<SDL2/SDL_vulkan.h>
#include<vulkan/vulkan.hpp>
#include"VulkanInitHeaders.h"
#include"Utils.h"


SDL_Window* window;
vk::Instance instance;
vk::SurfaceKHR surface;
vk::DebugUtilsMessengerEXT debugMessenger(nullptr);
vk::DispatchLoaderDynamic dldi;
vk::PhysicalDevice physicalDevice;
vk::Device vDevice;
std::vector<vk::Image> images;
std::vector<vk::ImageView> imagesView;
std::vector<vk::Framebuffer> frameBuffers;
std::vector<vk::CommandBuffer> commandBuffers;
vk::SurfaceFormatKHR format;
vk::RenderPass mainRenderPass;
vk::Pipeline graphicsPipeline;
vk::SwapchainKHR swapchain;
vk::Extent2D swapChainExtent;
vk::CommandPool commandPool;


std::vector<vk::Semaphore> imagesAviablesSemaphores;
std::vector<vk::Semaphore> renderFinishedSemaphores;
std::vector<vk::Fence> inFlightFences;


int currentFrame = 0;


struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
};


//gestor de los mensajes de debug de las layers
void makeDebugMessenger() {
	debugMessenger = make_debug_messenger( instance, dldi );
}

void createSyncObj() {
	vk::SemaphoreCreateInfo semaphoreInfo;
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.setFlags( vk::FenceCreateFlagBits::eSignaled );


	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT;i++) {
		imagesAviablesSemaphores.push_back( vDevice.createSemaphore( semaphoreInfo ));
		renderFinishedSemaphores.push_back( vDevice.createSemaphore( semaphoreInfo ) );
		inFlightFences.push_back( vDevice.createFence( fenceInfo ) );
	}

}

std::pair<std::array<vk::VertexInputAttributeDescription,2>,vk::VertexInputBindingDescription> createVertexData() {
	//conexion con un buffer y su disposicion de como sera leido, se podrian tener distintos buffers para agilizar el acceso a los datos
	vk::VertexInputBindingDescription binding;
	binding.binding = 0;
	binding.stride = sizeof( Vertex );
	binding.inputRate = vk::VertexInputRate::eVertex;

	//componente de posicion
	vk::VertexInputAttributeDescription attribute;
	attribute.binding = 0;
	attribute.location = 0;
	attribute.format = vk::Format::eR32G32B32Sfloat;
	attribute.offset = offsetof( Vertex, pos );

	//componente de color
	vk::VertexInputAttributeDescription attribute1;
	attribute.binding = 0;
	attribute.location = 1;
	attribute.format = vk::Format::eR32G32B32Sfloat;
	attribute.offset = offsetof(Vertex,color);


	return { {attribute,attribute1},binding };
}

vk::RenderPass createRenderPass() {

	vk::AttachmentDescription colorAttachment;
	colorAttachment.setFormat( format.format );
	colorAttachment.setSamples( vk::SampleCountFlagBits::e1 );
	colorAttachment.setLoadOp( vk::AttachmentLoadOp::eClear );
	colorAttachment.setStoreOp( vk::AttachmentStoreOp::eStore );
	colorAttachment.setStencilLoadOp( vk::AttachmentLoadOp::eDontCare );
	colorAttachment.setStencilStoreOp( vk::AttachmentStoreOp::eDontCare );
	colorAttachment.setInitialLayout( vk::ImageLayout::eUndefined );
	colorAttachment.setFinalLayout( vk::ImageLayout::ePresentSrcKHR );
	
	//referencia que se le da a los subpases para saber su configuracion de entrada y salida
	vk::AttachmentReference colorAttachmentRef;
	//inida el indice en la lista que se le va a dar el subpase para identificar el attachment
	colorAttachmentRef.setAttachment( 0 );
	colorAttachmentRef.setLayout( vk::ImageLayout::eColorAttachmentOptimal );

	vk::SubpassDescription subpassDesc;
	subpassDesc.setPipelineBindPoint( vk::PipelineBindPoint::eGraphics );
	subpassDesc.setColorAttachments( colorAttachmentRef );

	vk::SubpassDependency dependency;
	dependency.setSrcSubpass( vk::SubpassExternal );
	dependency.setDstSubpass( 0 );
	dependency.setSrcStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
	dependency.setDstStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
	dependency.setSrcAccessMask( vk::AccessFlagBits::eNone );
	dependency.setDstAccessMask( vk::AccessFlagBits::eColorAttachmentWrite );


	vk::RenderPassCreateInfo passCreateInfo;
	passCreateInfo.setAttachments( colorAttachment );
	passCreateInfo.setSubpasses( subpassDesc );
	passCreateInfo.setDependencies( dependency );


	return vDevice.createRenderPass(passCreateInfo);
}

vk::Pipeline createGraphicsPipeline() {

	vk::ShaderModule vertShaderMoudle = createShaderModule( readFile( "shaders/vert.spv" ) );
	vk::ShaderModule fragShaderMoudle = createShaderModule( readFile( "shaders/frag.spv" ) );

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.setModule( vertShaderMoudle );
	vertShaderStageInfo.setStage( vk::ShaderStageFlagBits::eVertex );
	vertShaderStageInfo.setPName( "main" );

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.setModule( fragShaderMoudle );
	fragShaderStageInfo.setStage( vk::ShaderStageFlagBits::eFragment );
	fragShaderStageInfo.setPName( "main" );

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo,fragShaderStageInfo };

	auto vertexData = createVertexData();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.setVertexAttributeDescriptions( vertexData.first );
	vertexInputInfo.setPVertexBindingDescriptions(&vertexData.second);

	//establece como se van a pintar la geometria: tirangulos, lienas, puntos, etc...
	vk::PipelineInputAssemblyStateCreateInfo vertexAssembly;
	vertexAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);
	vertexAssembly.setPrimitiveRestartEnable( vk::False );

	//Posiblemente haga falta un state de viewport

	//estado en el que se rellenan los pixeles a partir de la geometria propuesta
	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.setCullMode( vk::CullModeFlagBits::eFront );
	rasterizer.setFrontFace( vk::FrontFace::eCounterClockwise );
	rasterizer.setDepthClampEnable( vk::False );
	rasterizer.setRasterizerDiscardEnable( vk::False );
	rasterizer.setDepthBiasEnable( vk::False );

	vk::PipelineColorBlendAttachmentState attachtment;
	attachtment.setAlphaBlendOp( vk::BlendOp::eAdd );
	attachtment.setBlendEnable( vk::True );
	attachtment.setColorWriteMask( vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA );

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.setAttachments( attachtment );
	colorBlending.setLogicOpEnable( vk::False );

	vk::PipelineLayoutCreateInfo layout;
	
	vk::PipelineLayout pipelineLayout = vDevice.createPipelineLayout( layout );

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.setLayout( pipelineLayout );
	pipelineInfo.setStages( shaderStages );
	pipelineInfo.setPVertexInputState( &vertexInputInfo );
	pipelineInfo.setPInputAssemblyState( &vertexAssembly );
	pipelineInfo.setPRasterizationState( &rasterizer );
	pipelineInfo.setPColorBlendState( &colorBlending );
	pipelineInfo.setRenderPass(mainRenderPass);
	pipelineInfo.setSubpass( 0 );

	vDevice.destroyShaderModule( vertShaderMoudle );
	vDevice.destroyShaderModule( fragShaderMoudle );

	vk::ResultValue a = vDevice.createGraphicsPipeline( vk::PipelineCache( nullptr ), pipelineInfo );
	return a.value;
}


vk::SwapchainKHR createSwapChain(const vk::Device& device) {
	vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR( surface );

	auto formats = physicalDevice.getSurfaceFormatsKHR( surface );

	auto formatIterator = std::find_if( formats.begin(), formats.end(), []( vk::SurfaceFormatKHR form ) 
				  {
					  return (form.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
							   form.format == vk::Format::eB8G8R8A8Srgb);
				  } );

	format = (*formatIterator);

	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;


	vk::Extent2D extent = capabilities.currentExtent;

	vk::SwapchainCreateInfoKHR info;
	info.setSurface(surface);
	info.setMinImageCount( 2 );
	info.setImageFormat( format.format );
	info.setImageColorSpace( format.colorSpace );
	info.setImageExtent( extent );
	info.setImageArrayLayers( 1 );
	info.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment );
	info.setImageSharingMode( vk::SharingMode::eExclusive );
	info.setPreTransform( capabilities.currentTransform );
	info.setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque );
	info.setPresentMode( presentMode );
	info.setClipped( vk::True );

	

	return device.createSwapchainKHR( info );

}

void createFrameBuffres() {
	frameBuffers.resize( imagesView.size() );

	for (size_t i = 0; i < imagesView.size(); i++) {
		VkImageView attachments[] = {
				imagesView[i]
		};

		vk::FramebufferCreateInfo frameBufferInfo;

		frameBufferInfo.setAttachments( imagesView[i]);
		frameBufferInfo.setLayers( 1 );
		frameBufferInfo.setHeight(swapChainExtent.height);
		frameBufferInfo.setWidth(swapChainExtent.width);
		frameBufferInfo.setRenderPass( mainRenderPass );

		frameBuffers[i] = vDevice.createFramebuffer(frameBufferInfo);
	}
}

void createCoomandPool() {
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
	poolInfo.setQueueFamilyIndex( 0 );

	commandPool = vDevice.createCommandPool( poolInfo );
}

void init() {
	if (SDL_Init( SDL_INIT_VIDEO ) != 0) {

		throw std::runtime_error( "Could not initialize SDL." );
	}

	window =
		SDL_CreateWindow( "VulkanWindow",
						  SDL_WINDOWPOS_CENTERED,
						  SDL_WINDOWPOS_CENTERED,
						  1280, 720,
						  SDL_WINDOW_VULKAN );

	if (window == NULL) {
		throw std::runtime_error( "Could not create SDL window." );
	}

	unsigned extension_count;
	if (!SDL_Vulkan_GetInstanceExtensions( window, &extension_count, NULL )) {
		throw std::runtime_error( "Could not get the number of required instance extension from SDL." );
	}

	std::vector<const char*> extensions( extension_count );
	if (!SDL_Vulkan_GetInstanceExtensions( window, &extension_count, extensions.data() )) {
		throw std::runtime_error( "Could not get the names of required instance extensions form SDL." );
	}
	extensions.push_back( "VK_EXT_debug_utils" );

	std::vector<const char*> layers;

#if defined(_DEBUG)
	layers.push_back( "VK_LAYER_KHRONOS_validation" );
#endif

	vk::ApplicationInfo appInfo = vk::ApplicationInfo()
		.setPApplicationName( "Test for render engine" )
		.setApplicationVersion( 1 )
		.setPEngineName( "LLEngine" )
		.setEngineVersion( 1 )
		.setApiVersion( VK_API_VERSION_1_0 );

	vk::InstanceCreateInfo instanceInfo = vk::InstanceCreateInfo()
		.setFlags( vk::InstanceCreateFlags() )
		.setPApplicationInfo( &appInfo )
		.setEnabledExtensionCount( static_cast<uint32_t>(extensions.size()) )
		//.setPEnabledExtensionNames( extensions.data() )
		.setEnabledLayerCount( static_cast<uint32_t>(layers.size()) );
	//.setPEnabledLayerNames( layers.data() );
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	instanceInfo.ppEnabledLayerNames = layers.data();


	try {
		instance = vk::createInstance( instanceInfo );
	}
	catch (const std::exception& e) {
		std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
		//return 1;
	}



	//carga dinamica del proceso
	dldi = vk::DispatchLoaderDynamic( instance, vkGetInstanceProcAddr );

	VkSurfaceKHR c_surface;
	if (!SDL_Vulkan_CreateSurface( window, static_cast<VkInstance>(instance), &c_surface )) {
		throw std::runtime_error( SDL_GetError() );
	}

	surface = c_surface;

#ifdef _DEBUG
	makeDebugMessenger();
#endif // _DEBUG

	pickPhysicalDevice( instance, physicalDevice );

	vDevice = createLogicalDevice( physicalDevice, deviceExtensions );

	vk::Flags<vk::QueueFlagBits> flags = vk::QueueFlagBits::eGraphics & vk::QueueFlagBits::eTransfer;
	int32_t index =  findQueueFamilies( physicalDevice, flags );
	vk::Queue graphicsTransferQueue = vDevice.getQueue( index, 0 );
	
	swapchain =  createSwapChain( vDevice );

	images = vDevice.getSwapchainImagesKHR( swapchain );

	imagesView.resize( images.size() );
	for (int i = 0; i < images.size();i++) {
		vk::ImageViewCreateInfo viewInfo;
		viewInfo.setImage( images[i] );
		viewInfo.setViewType( vk::ImageViewType::e2D );
		viewInfo.setFormat( format.format );
		vk::ImageSubresourceRange range;
		range.setAspectMask( vk::ImageAspectFlagBits::eColor );
		range.setBaseMipLevel( 0 );
		range.setLevelCount( 1 );
		range.setBaseArrayLayer( 0 );
		range.setLayerCount( 1 );
		viewInfo.setSubresourceRange( range );
		vk::ComponentMapping componnets;
		componnets.setA( vk::ComponentSwizzle::eIdentity );
		componnets.setR( vk::ComponentSwizzle::eIdentity );
		componnets.setG( vk::ComponentSwizzle::eIdentity );
		componnets.setB( vk::ComponentSwizzle::eIdentity );
		viewInfo.setComponents( componnets );

		imagesView[i] = vDevice.createImageView( viewInfo );
	}
	
	mainRenderPass = createRenderPass();

	graphicsPipeline =createGraphicsPipeline();

	createFrameBuffres();

	createCoomandPool();

	//createUniformsBUffers, descriptorPools  cuando haya que usar 3D

	//creamos los bufers de commandos
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.setCommandPool( commandPool );
	allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
	allocInfo.setCommandBufferCount( 2 );

	commandBuffers =  vDevice.allocateCommandBuffers( allocInfo );

	createSyncObj();
}

void run() {
	// Poll for user input.
	bool stillRunning = true;
	vk::SubmitInfo submitInfo;


	while (stillRunning) {

		SDL_Event event;
		while (SDL_PollEvent( &event )) {

			switch (event.type) {

			case SDL_QUIT:
				stillRunning = false;
				break;

			default:
				// Do nothing.
				break;
			}
		}

		vDevice.waitForFences(inFlightFences[currentFrame], vk::True, UINT64_MAX);

		auto imageResult = vDevice.acquireNextImageKHR( swapchain, UINT64_MAX, imagesAviablesSemaphores[currentFrame], vk::Fence( nullptr ) );


		vDevice.resetFences( inFlightFences[currentFrame] );

		commandBuffers[currentFrame].reset();





		SDL_Delay( 10 );
	}
}

void quit() {
	// Clean up.
	instance.destroySurfaceKHR( surface );
	SDL_DestroyWindow( window );
	SDL_Quit();
	instance.destroy();
}

int main() {

	init();

	run();

	quit();

	return 0;
}