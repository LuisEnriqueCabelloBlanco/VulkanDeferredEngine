//#include "RenderEngine.h"
//
//
//bool RenderEngine::init()
//{
//    _window.init( "Proyecto Vulkan", 800, 600, instance );
//    createInstance();
//    setupDebugMessenger();
//    _window.createSurface( instance );
//    _device.init( instance, _window );
//    auto indices = _device.getFamilyIndexes();
//    graphicsQueue = _device.getGraphicsQueue();
//    presentQueue = _device.getPresentQueue();
//    trasferQueue = _device.getTransferQueue();
//    _window.setDevice( &_device );
//    _window.createSwapChain();
//
//
//    //createSwapChain();
//    //createImageViews();
//    //createDeferredRenderPass
//    createRenderPass();
//    //createDeferredDescriptorSetS
//    createDescriptorSetLayout();
//    //createDeferredPipelines
//    createGraphicsPipeline();
//
//    createCommandPool();
//    createColorResources();
//    createDepthResources();
//
//    //createDeferredFrameBuffers
//
//    createFramebuffers();
//
//    createCommandBuffers();
//    createSyncObjects();
//
//	return false;
//}