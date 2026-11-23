#include "SwapChain.h"


SwapChain::SwapChain( VulkanDevice* device, const VkSurfaceKHR& surface ):_deviceRef(device), _currentBuffer(0) {
	uint32_t numImages;

	_swapChain = _deviceRef->createSwapChain( surface,_framesFormat,_viewPortExtent,_images);
	
	_imageViews.resize( _images.size() );

	for (size_t i = 0; i < _imageViews.size(); i++) {
		_imageViews[i] = device->createImageView( _images[i], _framesFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
	}


}

SwapChain::~SwapChain()
{

	_deviceRef->destroySwapchain( _swapChain );
}
