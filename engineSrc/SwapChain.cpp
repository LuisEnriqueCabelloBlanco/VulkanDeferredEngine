#include "SwapChain.h"


SwapChain::SwapChain( VulkanDevice* device, const VkSurfaceKHR& surface ):_deviceRef(device), _currentBuffer(0) {
	uint32_t numImages;
	std::vector<VkImage> images;
	_swapChain = _deviceRef->createSwapChain( surface,_framesFormat,_viewPortExtent,images);
	
	_imageViews.resize( images.size() );

	for (size_t i = 0; i < _imageViews.size(); i++) {
		_imageViews[i] = device->createImageView( images[i], _framesFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
	}


}

SwapChain::~SwapChain()
{

	_deviceRef->destroySwapchain( _swapChain );
}
