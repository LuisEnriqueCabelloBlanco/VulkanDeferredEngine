#pragma once
#include<vulkan/vulkan.h>
//#include"VulkanDevice.h"

class VulkanDevice;


/**
 * @brief Clase buffer responsable de destruir los datos asociados
 */
class Buffer
{
	friend VulkanDevice;
public: 
	Buffer( VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, VulkanDevice* device );
	~Buffer();

	inline VkBuffer getBuffer() { return _buffer; };
	inline VkDeviceMemory getMemory() { return _bufferMemory; }
	void resize( VkDeviceSize size);

	void append( const Buffer& buffer );
private:
	VkBuffer _buffer;
	VkDeviceMemory _bufferMemory;

	VkDeviceSize _size;
	VkBufferUsageFlagBits _usage; 
	VkMemoryPropertyFlags _properties;
	
	VulkanDevice* _device;
};

