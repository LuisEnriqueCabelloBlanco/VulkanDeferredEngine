#include "VulkanWindow.h"
#include <stdexcept>
#include<SDL2/SDL.h>
#include<SDL2/SDL_vulkan.h>
#include "VulkanDevice.h"
#include <limits>
#include <algorithm> // Necessary for std::clamp

void VulkanWindow::init(const std::string& windowName, uint32_t w, uint32_t h, VkInstance instance)
{
    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS );
    //this->_instance = _instance;
    _width = w;
    _heith = h;
    _window = SDL_CreateWindow( windowName.data(), 100, 100, w, h, SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE );
    //createSurface(_instance);
}



void VulkanWindow::createSwapChain()
{
    VulkanDevice::SwapChainSupportDetails swapChainSupport = _device->getSwapChainSupportDetails(_surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);


    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }


    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


    VulkanDevice::QueueFamilyIndices indices = _device->_familyIndx;
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;


    if (vkCreateSwapchainKHR(_device->_device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(_device->_device, _swapchain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device->_device, _swapchain, &imageCount, _swapChainImages.data());

    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

    _swapChainImageViews.resize( _swapChainImages.size() );
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        _swapChainImageViews[i] = _device->createImageView( _swapChainImages[i], _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
    }
}

void VulkanWindow::createSurface(VkInstance& instance)
{
    //creamos la superficie para la ventana
    
    _instance = instance;

    if (SDL_Vulkan_CreateSurface( _window, instance, &_surface ) == SDL_bool::SDL_FALSE) {
        throw std::runtime_error(SDL_GetError());
    }
}

void VulkanWindow::cleanUpSwapChain()
{
    for (auto imageView : _swapChainImageViews) {
        _device->destroyImageView( imageView );
    }

    _device->destroySwapchain( _swapchain );
}

void VulkanWindow::framebufferResizeCallback(SDL_Window* window, int width, int height)
{
    //auto app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
    //app->_framebufferResized = true;
    //app->_width = width;
    //app->_heith = height;
}

VkSurfaceFormatKHR VulkanWindow::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

void VulkanWindow::close()
{
    cleanUpSwapChain();

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    SDL_DestroyWindow( _window );
}

VkPresentModeKHR VulkanWindow::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    //Hacer esto no te da doble buffering fluido
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanWindow::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        SDL_GetWindowSize( _window, &width, &height );
        //glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
