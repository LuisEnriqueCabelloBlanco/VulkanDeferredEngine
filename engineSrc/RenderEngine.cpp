#include "RenderEngine.h"
#include "Utils.h"
#include "Mesh.h"

// ============================
// Lifecycle
// ============================

void RenderEngine::wait() {
	_device.wait();
}

void RenderEngine::cleanup()
{
	_device.wait();

	if (_gbuffer) {
		_gbuffer->destroy();
		_gbuffer.reset();
	}

	if (_shadowPass) {
		_shadowPass->destroy();
		_shadowPass.reset();
	}

	_resources.releaseAllMaterials();
	_resources.releaseAllTextures();
	_resources.releaseAllMeshes();
	_resources.clear();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		delete uniformBuffers[i];
	}

	_window.destroySwapChain();

	delete _lightUniformBuffer;
	delete _lightBufferStorage;
	delete _lightIndexStorage;
	delete _AABBModelStorage;
	delete _lightCulledObjectIndex;
	delete _cameraCulledObjectIndex;
	delete _mainLightVPBuffer;

	_descriptors.destroy();
	_mainRenderPass.destroy();

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

	_device.close();
	_window.destroySurface(_vulkanInstance.getInstance());
	_vulkanInstance.destroy();
	_window.close();
}

RenderEngine::RenderEngine() : _resources( _device )
{
	_resources.setTextureBindingsChangedCallback( [this]() {
		_device.wait();
		updateGeometryDescriptors();
	} );
}

void RenderEngine::init( const std::string& appName )
{
	_window.init(appName, WIDTH, HEIGHT);
	_vulkanInstance.init( appName );	
	_window.createSurface( _vulkanInstance.getInstance() );
	_device.init( _vulkanInstance.getInstance(), _window );

	graphicsQueue = _device.getGraphicsQueue();
	presentQueue = _device.getPresentQueue();
	computeQueue = _device.getComputeQueue();
	_window.setDevice( &_device );
	_window.createSwapChain();


	_shadowPass = std::make_unique<ShadowPass>(_device, VkExtent2D{ 512, 512 });
	_shadowPass->create();


	_mainRenderPass.init( _device, _window );

	_gbuffer = std::make_unique<GBuffer>( _device, _window, _mainRenderPass.get() );
	_gbuffer->create();

	_pipelines.init( _device, _mainRenderPass.get(), _shadowPass->getRenderPass() );


	createCommandPool();
	createCommandBuffers();
	createSyncObjects();

	//cracion de recursos

	createUniformBuffers();
	createLightBuffer();
	createCullingBuffers();

	_mainCamera = Camera( glm::vec3( 0, 0, -2.5f ), glm::vec3( 0, 0, 1.f ), glm::vec3( 0.0f, 1.0f, 0.0f ),
				  90.f, _window.getExtent().width / (float)_window.getExtent().height, 0.1f, 40.f );

	// Inicializamos el DescriptorManager
	_descriptors.init( _device, _pipelines );

	updateComputeDescriptorSet();
	updateShadowDescriptorSet();
	updateCullDescriptorSet();
	updateGeometryDescriptors();
	updateLightingDescriptors();

}

void RenderEngine::createCommandPool()
{
	VulkanDevice::QueueFamilyIndices queueFamilyIndices = _device.getFamilyIndexes();

	commandPool = _device.createCommandPool(
    	VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    	queueFamilyIndices.graphicsFamily.value()
	);
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

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelines.getComputePipeline());

	VkDescriptorSet computeSets[] = {_descriptors.getLightsDataBufferSet(), _descriptors.getComputeSet(currentFrame) };

	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelines.getComputeLayout(), 0, 2, computeSets, 0, nullptr);

	vkCmdDispatch( commandBuffer, (MAX_LIGHTS/32)+1, 1, 1 );

	vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &memBarr, 0, nullptr );

	recordShadowPass( commandBuffer, objectsArray );
	recordMainRender( commandBuffer, imageIndex, objectsArray, cullIndex );

	if (vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS) {
		throw std::runtime_error( "failed to record command buffer!" );
	}
}

void RenderEngine::recordMainRender( VkCommandBuffer commandBuffer, uint32_t imageIndex,
	const std::vector<RenderObject>& objectsArray, const std::vector<int>& cullIndex )
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.framebuffer = _gbuffer->getFramebuffers()[imageIndex];
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _mainRenderPass.get();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = _window.getExtent();

	std::array<VkClearValue, 5> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[3].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
	clearValues[4].color = { {0.0f, 0.0f, 0.0f, 0.0f} };

	renderPassInfo.clearValueCount = static_cast<uint32_t>( clearValues.size() );
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

	VkDescriptorSet descriptors[] = { _descriptors.getCameraSet(currentFrame), _descriptors.getTextureArraySet() };
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.getGraphicsLayout(), 0, 2, descriptors, 0, nullptr );
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.getGraphicsPipeline() );

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>( _window.getExtent().width );
	viewport.height = static_cast<float>( _window.getExtent().height );
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
		_descriptors.getInputAttachmentsSet(currentFrame),
		_descriptors.getMainLightSet(),
		_descriptors.getLightsDataBufferSet(),
		_descriptors.getGlobalLightingSet(currentFrame)
	};

	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.getDeferredLayout(), 0, 4, sets, 0, nullptr );
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.getDeferredPipeline() );

	if (cullIndex.size() > 0) {
		vkCmdDraw( commandBuffer, 3, 1, 0, 0 );
	}

	vkCmdEndRenderPass( commandBuffer );
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
	if ( _swapChainRecreationPending ) {
		performSwapChainRecreation();
		return;
	}

	const std::vector<RenderObject>& objectsArray = _scene.buildRenderQueue();
	const std::vector<LightObject>& lightQueue = _scene.buildLightQueue();

	_device.waitForFences(inFlightFences[currentFrame]);
	uint32_t imageIndex;
	VkResult result = _device.adquireNextImage(
		_window.getSwapChain(), imageAviablesSemaphores[currentFrame], imageIndex);


	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		performSwapChainRecreation();
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

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _swapChainRecreationPending)
	{
		performSwapChainRecreation();
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
	_device.wait();
	if ( _gbuffer ) {
		_gbuffer->destroy();
	}

	VkFormat oldFormat = _window.getSwapChainFormat();
	_window.destroySwapChain();
	_window.createSwapChain();

	// Si el formato del swapchain cambia, hay que recrear el main render pass y los pipelines que dependen de el.
	// Raro pero posible
	if ( _window.getSwapChainFormat() != oldFormat ) {
		_mainRenderPass.destroy();
		_mainRenderPass.init( _device, _window );
		_pipelines.destroy();
		_pipelines.init( _device, _mainRenderPass.get(), _shadowPass->getRenderPass() );
	}

	_mainCamera.setAspectRatio( _window.getExtent().width / static_cast<float>( _window.getExtent().height ) );
	_gbuffer->create();
}

void RenderEngine::performSwapChainRecreation()
{
	recreateSwapChain();
	updateLightingDescriptors();
	_swapChainRecreationPending = false;
}

void RenderEngine::updateUniformBuffer(uint32_t currentImage, glm::mat4 model) {
	ViewProjectionData ubo{};
	ubo.view = _mainCamera.getViewMatrix();
	ubo.proj = _mainCamera.getProjMatrix();
	ubo.proj[1][1] *= -1;

	// Escribimos los datos calculados en el UBO mapeado del frame actual
	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

	// Actualizamos datos de iluminación dependientes de la cámara
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
	_descriptors.updateCompute( _lightUniformBuffer );
}

void RenderEngine::updateShadowDescriptorSet()
{
	_descriptors.updateShadow( _mainLightVPBuffer );
}

void RenderEngine::updateCullDescriptorSet()
{
	_descriptors.updateCulling( _AABBModelStorage,
								 _lightIndexStorage,
								 _lightBufferStorage,
								_cameraCulledObjectIndex,
								_lightCulledObjectIndex );
}

void RenderEngine::updateGeometryDescriptors()
{
	_descriptors.updateGeometry( uniformBuffers,
								 _resources.getLiveTextureEntries() );
}

void RenderEngine::updateLightingDescriptors()
{
	_descriptors.updateLighting( *_gbuffer,
								 *_shadowPass,
								 _lightUniformBuffer );
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
		_swapChainRecreationPending = true;
	}
	_window.handleWindowEvent( event );
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


void RenderEngine::pushModelMatrix( VkCommandBuffer commnadBuffer, glm::mat4 model )
{
	vkCmdPushConstants( commnadBuffer, _pipelines.getGraphicsLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &model );
}

void RenderEngine::pushTextureIndex( VkCommandBuffer commnadBuffer, const MaterialData& material )
{
	vkCmdPushConstants( commnadBuffer, _pipelines.getGraphicsLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof( glm::mat4 ), sizeof( material ), &material );
}

void RenderEngine::recordShadowPass( VkCommandBuffer commandBuffer, const std::vector<RenderObject>& objectsArray )
{
	// Si no hay main light designada o fue destruida, no hay shadow pass.
	if (!_scene.hasMainLight()) {
		return;
	}

	std::vector<int> cullIndex = cullObjects(objectsArray, *_mainLightVPMapped);

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.framebuffer = _shadowPass->getFramebuffer();
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _shadowPass->getRenderPass();
	renderPassInfo.renderArea.offset = { 0, 0 };
	const Texture* shadowMap = _shadowPass->getShadowMap();
	VkExtent2D ext{ shadowMap->texWidth, shadowMap->texHeight };
	renderPassInfo.renderArea.extent = ext;

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	//tomamos los datos de paso de renderizado
	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
	VkDescriptorSet shadowSet = _descriptors.getMainLightSet();
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.getShadowLayout(), 0, 1, &shadowSet, 0, nullptr );
	//Aplicamos la pipeline definida
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.getShadowPipeline());

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
	// El renderpass de sombras automáticamente ha transicionado la imagen a DEPTH_STENCIL_READ_ONLY_OPTIMAL
	// según su finalLayout declarado, lista para lectura en el lighting pass
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
