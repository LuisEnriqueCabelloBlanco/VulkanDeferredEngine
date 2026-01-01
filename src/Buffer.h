#pragma once
#include<vulkan/vulkan.h>
#include"VulkanDevice.h"

class Buffer
{
public: 
	Buffer( VulkanDevice& device );
	~Buffer();



private:
	VkBuffer _buffer;
	VkDeviceMemory _bufferMemory;
};

