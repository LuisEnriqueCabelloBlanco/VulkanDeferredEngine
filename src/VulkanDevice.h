#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include "VulkanWindow.h"


const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME/*,
	VK_KHR_SURFACE_EXTENSION_NAME*/
};

class VulkanDevice
{
	friend VulkanWindow;
public:
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> computeFamily;
		std::optional<uint32_t> transferFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value() /*&& presentFamily.has_value()*/;
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	VulkanDevice() {};
	~VulkanDevice();



	void init( VkInstance instance, VulkanWindow& window );


	/// <summary>
	/// devuelve el soporte para una swapchain que se ajueste a la superficie indicada
	/// </summary>
	/// <param name="surface"></param>
	/// <returns></returns>
	SwapChainSupportDetails getSwapChainSupportDetails( VkSurfaceKHR surface );

	void close();




#pragma region Getters & Setters
	const QueueFamilyIndices getFamilyIndexes() const { return _familyIndx; }
	VkQueue getGraphicsQueue();
	VkQueue getPresentQueue();
	VkQueue getTransferQueue();
	VkSampleCountFlagBits getMssaSamples() const { return msaaSamples; }

#pragma endregion


#pragma region Creadores
	VkImage createImage( uint32_t width, uint32_t height, uint32_t mipLvl, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties );

	VkImageView createImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels );

	VkShaderModule createShaderModule( const std::vector<char>& code );

	VkPipelineLayout createPipelineLayout( VkPipelineLayoutCreateInfo& info, VkAllocationCallbacks* pAllocator = nullptr );

	VkDescriptorSetLayout createDescriptorSetLayout( VkDescriptorSetLayoutCreateInfo& createInfo, VkAllocationCallbacks* pAllocator = nullptr );

	VkSwapchainKHR createSwapChain( const VkSurfaceKHR& surface, VkFormat& format, VkExtent2D& extent, std::vector<VkImage>& images );

	std::vector<VkPipeline> createPipelines( VkPipelineCache pipelineCache, std::vector<VkGraphicsPipelineCreateInfo> createInfos, VkAllocationCallbacks* pAllocator = nullptr );

	std::vector<VkCommandBuffer> createCommandBuffers( VkCommandPool comandPool, VkCommandBufferLevel level, uint32_t numCommandBuffers );

	VkCommandPool createCommandPool( VkCommandPoolCreateFlags flags, uint32_t familyIndex, VkAllocationCallbacks* pAllocator = nullptr );

	VkDescriptorPool createDescriptorPool( VkDescriptorPoolCreateInfo& createInfo, VkAllocationCallbacks* pAllocator = nullptr );

	VkFramebuffer createFrameBuffer( VkFramebufferCreateInfo& createInfo, VkAllocationCallbacks* pAllocator = nullptr );

	VkSemaphore createSemaphore( VkSemaphoreCreateInfo createInfo, VkAllocationCallbacks* pAllocator = nullptr );

	VkRenderPass createRenderPass( VkRenderPassCreateInfo& createInfo, VkAllocationCallbacks* pAllocator = nullptr );

	VkFence createFence( VkFenceCreateInfo createInfo, VkAllocationCallbacks* pAllocator = nullptr );

	void createBuffer( VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory );

	inline VkSampler createSampler( VkSamplerCreateInfo& info ) {
		VkSampler sampler;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties( _physicalDevice, &properties );

		info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		if (vkCreateSampler( _device, &info, nullptr, &sampler ) != VK_SUCCESS) {
			throw std::runtime_error( "failed to create texture sampler!" );
		}

		return sampler;
	}

	std::vector<VkDescriptorSet>&& createDescriptorSets( const std::vector<std::vector<VkWriteDescriptorSet>>& descriptorWrites, const std::vector<VkDescriptorSetLayout>& layouts, VkDescriptorPool descriptorPool );

	void createDevice();
#pragma endregion


	inline void mapMemory( VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, void** ppData ) {
		vkMapMemory( _device, memory, offset, size, 0, ppData );
	}

	inline void unmapMemory( VkDeviceMemory memory ) {
		vkUnmapMemory( _device, memory );
	}

	inline void freeMemory( VkDeviceMemory mem, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkFreeMemory( _device, mem, pAllocator );
	}

	inline void freeCommandBuffers( VkCommandPool pool, std::vector<VkCommandBuffer>& commandBuffers ) {
		vkFreeCommandBuffers( _device, pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data() );
	}

#pragma region Destructores

	inline void destroyFrameBuffer( VkFramebuffer& buff, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyFramebuffer( _device, buff, pAllocator );
	}

	inline void destroyBuffer( VkBuffer& buff, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyBuffer( _device, buff, pAllocator );
	}

	inline void destroyImageView( VkImageView& imageView, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyImageView( _device, imageView, pAllocator );
	}

	inline void destroyShaderModule( VkShaderModule& module, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyShaderModule( _device, module, pAllocator );
	}

	inline void destroySwapchain( VkSwapchainKHR& swapchain, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroySwapchainKHR( _device, swapchain, pAllocator );
	}

	inline void destroyPipeline( VkPipeline& pipeline, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyPipeline( _device, pipeline, pAllocator );
	}

	inline void destroyPipelineLayout( VkPipelineLayout& pipeLayout, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyPipelineLayout( _device, pipeLayout, pAllocator );
	}

	inline void destroyRenderPass( VkRenderPass& renderPass, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyRenderPass( _device, renderPass, pAllocator );
	}

	inline void destroyCommandPool( VkCommandPool& commnadPool, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyCommandPool( _device, commnadPool, pAllocator );
	}

	inline void destroyDescriptorPool(VkDescriptorPool& descriptorPool, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyDescriptorPool( _device, descriptorPool, pAllocator );
	}

	inline void destroySemaphore( VkSemaphore& sem, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroySemaphore( _device, sem, pAllocator );
	}

	inline void destroyFence( VkFence& fence, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyFence( _device, fence, pAllocator );
	}

	inline void destroySampler( VkSampler sampler, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroySampler( _device, sampler, pAllocator );
	}

	inline void destroyImage( VkImage image, VkAllocationCallbacks* pAllocator = nullptr ) {
		vkDestroyImage( _device, image, pAllocator );
	}

#pragma endregion

	uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

	VkFormat findSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );

	inline void wait() {
		vkDeviceWaitIdle( _device );
	}

	inline void waitForFences( VkFence& fence ) {
		vkWaitForFences( _device, 1, &fence, VK_TRUE, UINT64_MAX );
	}

	inline void resetFences( VkFence& fence ) {
		vkResetFences( _device, 1, &fence );
	}

	inline VkResult adquireNextImage( VkSwapchainKHR swapChain, VkSemaphore& sem, uint32_t& index ) {
		return vkAcquireNextImageKHR( _device, swapChain, UINT64_MAX, sem, VK_NULL_HANDLE, &index );
	}

	/**
	 * @brief Reserva memoria y la vincula a una imagen
	 * @param image 
	 * @return 
	 */
	VkDeviceMemory bindMemoryToImage( VkImage image );

private:


	void pickPhysicalDevice();
	bool isDeviceSuitable( VkPhysicalDevice device );
	bool checkDeviceExtensionSupport( VkPhysicalDevice device );


	VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );
	VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );
	VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );


	/// <summary>
	/// busca los indices de las colas de un dispositivo físico cualquiera
	/// </summary>
	/// <param name="device"></param>
	/// <param name="window"></param>
	/// <returns></returns>
	QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device, VulkanWindow& window );

	VkSampleCountFlagBits getMaxUsableSampleCount();


	VkInstance _vkInstance;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;

	VulkanWindow* _window;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	QueueFamilyIndices _familyIndx;
};

