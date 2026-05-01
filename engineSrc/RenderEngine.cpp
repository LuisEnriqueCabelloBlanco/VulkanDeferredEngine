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

	_gbuffer.destroy();
	_shadowPass.destroy();

	_resources.releaseAllMaterials();
	_resources.releaseAllTextures();
	_resources.releaseAllMeshes();
	_resources.clear();

	_buffers.destroy();

	_window.destroySwapChain();

	_descriptors.destroy();
	_mainRenderPass.destroy();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		_device.destroySemaphore( renderFinishedSemaphores[i] );
		_device.destroySemaphore( imageAviablesSemaphores[i] );
		_device.destroyFence( inFlightFences[i] );
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

	_shadowPass.init( _device, VkExtent2D{ 512, 512 } );
	_shadowPass.create();


	_mainRenderPass.init( _device, _window );

	_gbuffer.init( _device, _window, _mainRenderPass.get() );
	_gbuffer.create();

	_pipelines.init( _device, _mainRenderPass.get(), _shadowPass.getRenderPass() );


	createCommandPool();
	createCommandBuffers();
	createSyncObjects();

	_buffers.init( _device );

	// Camera now lives inside Scene; set initial aspect ratio from window.
	_scene.setCameraAspectRatio(
		_window.getExtent().width / static_cast<float>( _window.getExtent().height ) );

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

void RenderEngine::recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<RenderObject>& objectsArray, const std::vector<int>&cameraIndex, const std::vector<int>&shadowIndex )
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
	memBarr.buffer = _buffers.getLightIndexStorage()->getBuffer();
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

	recordShadowPass( commandBuffer, objectsArray, shadowIndex );
	recordMainRender( commandBuffer, imageIndex, objectsArray, cameraIndex );

	if (vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS) {
		throw std::runtime_error( "failed to record command buffer!" );
	}
}

void RenderEngine::recordMainRender( VkCommandBuffer commandBuffer, uint32_t imageIndex,
	const std::vector<RenderObject>& objectsArray, const std::vector<int>& cullIndex )
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.framebuffer = _gbuffer.getFramebuffers()[imageIndex];
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
	const Camera& cam = _scene.getInternalCamera();
	camDesc.proj = cam.getProjMatrix();
	camDesc.view = cam.getViewMatrix();

	// Usamos CullManager para obtener el culling de los objetos
	// respecto a todos los frustums de una sola pasada.
	const bool hasShadows = _scene.hasMainLight();
	std::vector<ViewProjectionData> cullVPs;
	cullVPs.push_back( camDesc );
	if ( hasShadows ) {
		cullVPs.push_back( *_buffers.getMainLightVPMapped() );
	}

	auto cullResults = _culler.cullObjects( objectsArray, cullVPs, _resources );
	const std::vector<int>& cameraVisible = cullResults[0];
	const std::vector<int>& mainLightVisible = hasShadows ? cullResults[1] : cameraVisible;

	// Update de buffers de cámara, luces y VP de la main light
	ViewProjectionData camVP;
	camVP.view = cam.getViewMatrix();
	camVP.proj = cam.getProjMatrix();
	_buffers.writeCameraVP( currentFrame, camVP );

	_lighting.eyePos = cam.getPosition();
	_buffers.writeLighting( _lighting );

	// VP de la main light para shadow pass
	{
		ViewProjectionData lightVP;
		float ratio = _window.getExtent().width / static_cast<float>( _window.getExtent().height );
		constexpr float scale = 20.f;
		lightVP.proj = glm::ortho( -ratio * scale, ratio * scale, scale, -scale, 0.01f, 100.f );
		const LightObject* mainLight = _scene.tryGetMainLight();
		glm::vec3 lightDir = (mainLight != nullptr) ? mainLight->posOrDir : glm::vec3( 0.f, -1.f, 0.f );
		glm::vec3 pos = glm::vec3( 5.f, 10.f, -10.f ) + cam.getPosition();
		lightVP.view = glm::lookAt( pos, pos + lightDir, glm::vec3( 0, 1, 0 ) );
		_buffers.writeMainLightVP( lightVP );
	}

	_buffers.writeLights( lightQueue );
	_device.resetFences( inFlightFences[currentFrame] );

	/* DEBUG
	if (cameraVisible.size() == 0) {
		std::cout << "No hay objetos a pintar\n";
	}
	*/

	Mesh::_lastRenderedMesh = nullptr;
	vkResetCommandBuffer( commandBuffers[currentFrame], 0 );
	recordCommandBuffer( commandBuffers[currentFrame], imageIndex, objectsArray, cameraVisible, mainLightVisible );

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
	_gbuffer.destroy();

	VkFormat oldFormat = _window.getSwapChainFormat();
	_window.destroySwapChain();
	_window.createSwapChain();

	// Si el formato del swapchain cambia, hay que recrear el main render pass y los pipelines que dependen de el.
	// Raro pero posible
	if ( _window.getSwapChainFormat() != oldFormat ) {
		_mainRenderPass.destroy();
		_mainRenderPass.init( _device, _window );
		_pipelines.destroy();
		_pipelines.init( _device, _mainRenderPass.get(), _shadowPass.getRenderPass() );
	}

	_scene.setCameraAspectRatio( _window.getExtent().width / static_cast<float>( _window.getExtent().height ) );
	_gbuffer.create();
}

void RenderEngine::performSwapChainRecreation()
{
	recreateSwapChain();
	updateLightingDescriptors();
	_swapChainRecreationPending = false;
}

void RenderEngine::updateComputeDescriptorSet()
{
	_descriptors.updateCompute( _buffers.getLightingUBO() );
}

void RenderEngine::updateShadowDescriptorSet()
{
	_descriptors.updateShadow( _buffers.getMainLightVPBuffer() );
}

void RenderEngine::updateCullDescriptorSet()
{
	_descriptors.updateCulling( _buffers.getAABBStorage(),
						 	 _buffers.getLightIndexStorage(),
						 	 _buffers.getLightStorage(),
							_buffers.getCameraCullIndex(),
							_buffers.getLightCullIndex() );
}

void RenderEngine::updateGeometryDescriptors()
{
	_descriptors.updateGeometry( _buffers.getCameraBuffers(),
								 _resources.getLiveTextureEntries() );
}

void RenderEngine::updateLightingDescriptors()
{
	_descriptors.updateLighting( _gbuffer,
								 _shadowPass,
								 _buffers.getLightingUBO() );
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


void RenderEngine::pushModelMatrix( VkCommandBuffer commnadBuffer, glm::mat4 model )
{
	vkCmdPushConstants( commnadBuffer, _pipelines.getGraphicsLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &model );
}

void RenderEngine::pushTextureIndex( VkCommandBuffer commnadBuffer, const MaterialData& material )
{
	vkCmdPushConstants( commnadBuffer, _pipelines.getGraphicsLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof( glm::mat4 ), sizeof( material ), &material );
}

void RenderEngine::recordShadowPass( VkCommandBuffer commandBuffer, const std::vector<RenderObject>& objectsArray, const std::vector<int>& cullIndex )
{
	// Si no hay main light designada o fue destruida, no hay shadow pass.
	if (!_scene.hasMainLight()) {
		return;
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.framebuffer = _shadowPass.getFramebuffer();
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _shadowPass.getRenderPass();
	renderPassInfo.renderArea.offset = { 0, 0 };
	const Texture* shadowMap = _shadowPass.getShadowMap();
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
}