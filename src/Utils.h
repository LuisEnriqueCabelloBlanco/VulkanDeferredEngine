#pragma once

#include <vulkan/vulkan.h>
#include <fstream>
#include<vector>


struct SwapChainImageContext {
	VkImage image;
	VkCommandBuffer cmd;
	VkImageView view;
	VkBuffer uniformBuffer;
	VkDeviceMemory deviceMemory;
	void* uniform_memory_ptr;
	VkFramebuffer frameBuffer;
	VkDescriptorSet descriptor_set;
};

static std::vector<char> readFile( const std::string& filename ) {
	std::ifstream file( filename, std::ios::ate | std::ios::binary );

	if (!file.is_open()) {
		throw std::runtime_error( "failed to open file! " + filename );
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer( fileSize );
	file.seekg( 0 );
	file.read( buffer.data(), fileSize );
	file.close();

	return buffer;
}