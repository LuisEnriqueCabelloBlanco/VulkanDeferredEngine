#pragma once
#include<vector>
#include"Utils.h"
#include"vulkan/vulkan.h"
#include"VulkanDevice.h"

class SwapChain
{
	SwapChain(VulkanDevice* device , const VkSurfaceKHR& surface);
	~SwapChain();

private:


	VkSwapchainKHR _swapChain;
	//std::vector<SwapChainImageContext> _swapChainImages;

	std::vector<VkImageView> _imageViews;

	uint32_t _currentBuffer;
	VkFormat _framesFormat;
	VkExtent2D _viewPortExtent;
	VulkanDevice* _deviceRef;

};

