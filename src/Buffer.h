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
	Buffer( VkBuffer buffer, VkDeviceMemory bufferMem, VulkanDevice* device );
	~Buffer();

	inline VkBuffer getBuffer() { return _buffer; };
	inline VkDeviceMemory getMemory() { return _bufferMemory; }
private:
	VkBuffer _buffer;
	VkDeviceMemory _bufferMemory;

	VulkanDevice* _device;
};

