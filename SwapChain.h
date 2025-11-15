#pragma once
#include<vector>
#include"Utils.h"
#include"vulkan/vulkan.h"

class SwapChain
{
	

private:

	std::vector<SwapChainImageContext> _swapChainImages;
	uint32_t _currentBuffer;
	VkFormat _framesFormat;
	VkExtent2D _viewPortExtent;
};

