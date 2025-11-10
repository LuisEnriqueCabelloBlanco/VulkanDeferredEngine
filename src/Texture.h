#pragma once
#include "VulkanDevice.h"
class App;
class Texture
{
public:
	Texture(VulkanDevice& vkDv):device(vkDv),textureImage(), textureImageView(), textureImageMemory(),textureSampler(), texWidth(0), texHeight(0) {};
	~Texture();
	void loadTexture(const std::string& path, App& app);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createTextureSampler();

	uint32_t mipLevels;
	VulkanDevice& device;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	uint32_t texWidth, texHeight;
	VkSampler textureSampler;
	VkFormat mFormat = VK_FORMAT_R8G8B8A8_SRGB;
};

