#include "Buffer.h"
#include<vulkan/vulkan.hpp>
#include "VulkanDevice.h"

Buffer::Buffer( VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, VulkanDevice* device )
{
	_device = device;
	_size = size;
	_properties = properties;
	_usage = usage;
	_device->createBuffer( _size, _usage, _properties, _buffer, _bufferMemory );
}

Buffer::~Buffer()
{
	_device->destroyBuffer( _buffer );
	_device->freeMemory( _bufferMemory );
}

void Buffer::resize( VkDeviceSize size )
{
	VkBuffer auxBuffer;
	VkDeviceMemory auxMem;

	_device->createBuffer( size, _usage, _properties, auxBuffer, auxMem);

	_device->copyBuffer( _buffer, auxBuffer, _size );

	_device->destroyBuffer( _buffer );
	_device->freeMemory( _bufferMemory );

	_buffer = auxBuffer;
	_bufferMemory = auxMem;
	_size = 0;
}

