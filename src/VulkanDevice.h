#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include "VulkanWindow.h"


const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME/*,
	VK_KHR_SURFACE_EXTENSION_NAME*/
};

struct VulkanDevice
{
	friend VulkanWindow;
public:
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> computeFamily;
		std::optional<uint32_t> transferFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value()&& computeFamily.has_value()&&transferFamily.has_value() /*&& presentFamily.has_value()*/;
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};


	VulkanDevice() {};
	~VulkanDevice();

	void createDevice();

	void init(VkInstance instance, VulkanWindow& window);

	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;

	VulkanWindow* mWindow;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	QueueFamilyIndices familyIndx;
	/// <summary>
	/// devuelve el soporte para una swapchain que se ajueste a la superficie indicada
	/// </summary>
	/// <param name="surface"></param>
	/// <returns></returns>
	SwapChainSupportDetails getSwapChainSupportDetails(VkSurfaceKHR surface);

	void close();

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

private:
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	/// <summary>
	/// busca los indices de las colas de un dispositivo físico cualquiera
	/// </summary>
	/// <param name="device"></param>
	/// <param name="window"></param>
	/// <returns></returns>
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VulkanWindow& window);

	VkSampleCountFlagBits getMaxUsableSampleCount();
};

