#include "Buffer.h"
#include<vulkan/vulkan.hpp>
#include "VulkanDevice.h"

Buffer::Buffer( VkBuffer buffer, VkDeviceMemory bufferMem, VulkanDevice* device )
{
	_buffer = buffer;
	_bufferMemory = bufferMem;
	_device = device;
}

Buffer::~Buffer()
{
	_device->destroyBuffer( _buffer );
	_device->freeMemory( _bufferMemory );
}
