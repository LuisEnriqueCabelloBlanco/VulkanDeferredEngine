#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>
#include "WindowEvent.h"


struct SDL_Window;
class VulkanDevice;
class VulkanWindow
{
public:
	VulkanWindow() {};
	~VulkanWindow() {};

	void init(const std::string& windowName, uint32_t w, uint32_t h);
	void createSwapChain();
	void createSurface(VkInstance instance);

	void destroySurface(VkInstance instance);
	void destroySwapChain();
	void close();

	static void framebufferResizeCallback(SDL_Window* window, int width, int height);

	void setDevice(VulkanDevice* vkDv) { _device = vkDv; };

	SDL_Window* getWindow() { return _window; };

	VkSurfaceKHR getSurface() { return _surface; };

	VkFormat getSwapChainFormat() { return _swapChainImageFormat; }

	VkExtent2D getExtent() { return _swapChainExtent; }

	const std::vector<VkImageView> & getImageViews() { return _swapChainImageViews; }

	VkSwapchainKHR getSwapChain() { return _swapchain; }

	void handleWindowEvent( const WindowEvent& ev );
	
private:

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	
	SDL_Window* _window;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;

	std::vector<VkImage> _swapChainImages;
	std::vector<VkImageView> _swapChainImageViews;

	uint32_t _width;
	uint32_t _height;

	VulkanDevice* _device;

};

