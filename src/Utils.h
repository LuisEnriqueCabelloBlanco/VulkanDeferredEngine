#pragma once

#include <vulkan/vulkan.h>


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

struct ModelData {
	//std::vector<Vertex> vert;
	std::vector<uint32_t> indices;
};