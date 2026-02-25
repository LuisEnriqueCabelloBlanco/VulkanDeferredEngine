#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>


class SDL_Window;
class VulkanDevice;
class VulkanWindow
{
public:
	VulkanWindow() {};
	~VulkanWindow() {};

	void init(const std::string& windowName, uint32_t w, uint32_t h,VkInstance instance);

	/// <summary>
	/// Crea la swapchain para el VulkanDevice se debe llamar tras haber creado el device
	/// </summary>
	void createSwapChain();


	static void framebufferResizeCallback(SDL_Window* window, int width, int height);


	void setDevice(VulkanDevice* vkDv) { _device = vkDv; };

	void close();

	SDL_Window* getWindow() { return _window; };

	VkSurfaceKHR getSurface() { return _surface; };

	VkFormat getFormat() { return _swapChainImageFormat; }

	VkExtent2D getExtent() { return _swapChainExtent; }

	const std::vector<VkImageView> & getImageViews() { return _swapChainImageViews; }

	VkSwapchainKHR getSwapChain() { return _swapchain; }

	void createSurface(VkInstance& instance);

	void cleanUpSwapChain();
private:

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	
	SDL_Window* _window;
	VkInstance _instance;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;

	std::vector<VkImage> _swapChainImages;
	std::vector<VkImageView> _swapChainImageViews;

	uint32_t _width;
	uint32_t _heith;
	bool _framebufferResized = false;
	
	VulkanDevice* _device;

};

