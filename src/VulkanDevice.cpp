#include "VulkanDevice.h"
#include"BufferObjectsData.h"
#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>

//VulkanDevice::VulkanDevice(VkInstance _instance,VulkanWindow window):_vkInstance(_instance),windowRef(window)
//{
//    pickPhysicalDevice();
//}

VulkanDevice::~VulkanDevice()
{
    
}

void VulkanDevice::createDevice()
{
    _familyIndx = findQueueFamilies(_physicalDevice,*_window);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { _familyIndx.graphicsFamily.value(),_familyIndx.presentFamily.value(),_familyIndx.transferFamily.value()};


    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    //se crean las layers si es que hacen falta
    //if (enableValidationLayers) {
    //    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //    createInfo.ppEnabledLayerNames = validationLayers.data();
    //}
    //else {
    //    createInfo.enabledLayerCount = 0;
    //}

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    //se deberia crear una clase para la queues
    /*vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &presentQueue);*/
}

void VulkanDevice::init(VkInstance instance, VulkanWindow& window)
{
    _vkInstance = instance;
    _window = &window;
    pickPhysicalDevice();
    createDevice();
}

VulkanDevice::SwapChainSupportDetails VulkanDevice::getSwapChainSupportDetails(VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;
    //obtencion de las capacidades en funcion de la pantalla y el dispositivo
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, surface, &details.capabilities);

    //obtencion de los formatos soportados
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &formatCount, details.formats.data());
    }


    //obtencion de los modos soportados
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void VulkanDevice::close()
{
    vkDestroyDevice(_device, nullptr);
}

VkImage VulkanDevice::createImage( uint32_t width, uint32_t height, uint32_t mipLvl, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties )
{
    VkImage img;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLvl;
    imageInfo.arrayLayers = 1;
    /*Vulkan supports many possible image formats,
    but we should use the same format for the texels as the pixels in the buffer,
    otherwise the copy operation will fail.*/
    imageInfo.format = format;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = numSamples;
    imageInfo.flags = 0; // Optional
    if (vkCreateImage(_device, &imageInfo, nullptr, &img ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create image!" );
    }
    return img;
}

VkImageView VulkanDevice::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
    return imageView;
}

VkDeviceMemory VulkanDevice::bindMemoryToImage( VkImage image )
{
    VkDeviceMemory memory;

    //gestion de memoria
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements( _device, image, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    //App::findMemoryType(device._physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory( _device, &allocInfo, nullptr, &memory ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to allocate image memory!" );
    }

    vkBindImageMemory( _device, image, memory, 0 );

    return memory;
}

void VulkanDevice::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_vkInstance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)){
            _physicalDevice = device;
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}


bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device, *_window);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    //if (extensionsSupported) {
    //    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    //    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    //}

    return indices.isComplete() && extensionsSupported/* && swapChainAdequate*/;
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionsCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);

    std::vector<VkExtensionProperties> aviableExtensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, aviableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : aviableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

uint32_t VulkanDevice::findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties( _physicalDevice, &memProperties );

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error( "failed to find suitable memory type!" );
}

VkFormat VulkanDevice::findSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features )
{
        for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device, VulkanWindow& window)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    //Recorre y mira a que famialia se le puede asignar el dispositivo
    // Logic to find queue family indices to populate struct with
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.transferFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, window.getSurface(), &presentSupport);
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()&&supportedFeatures.samplerAnisotropy) {
            break;
        }
        i++;
    }
    return indices;
}

VkSampleCountFlagBits VulkanDevice::getMaxUsableSampleCount()
{

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(_physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

VkShaderModule VulkanDevice::createShaderModule( const std::vector<char>& code ) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create shader module!" );
    }

    return shaderModule;
}

VkPipelineLayout VulkanDevice::createPipelineLayout( VkPipelineLayoutCreateInfo& info, VkAllocationCallbacks* pAllocator )
{
    VkPipelineLayout layout;

    if (vkCreatePipelineLayout( _device, &info, pAllocator, &layout) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create pipeline layout!" );
    }

    return layout;
}

VkDescriptorSetLayout VulkanDevice::createDescriptorSetLayout( VkDescriptorSetLayoutCreateInfo& createInfo, VkAllocationCallbacks* pAllocator )
{
    VkDescriptorSetLayout layout;

    if (vkCreateDescriptorSetLayout( _device, &createInfo, pAllocator, &layout ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create descriptor set layout!" );
    }
    return layout;
}

VkSwapchainKHR VulkanDevice::createSwapChain( const VkSurfaceKHR& surface, VkFormat& format, VkExtent2D& extent, std::vector<VkImage>& images )
{
    VkSwapchainKHR swapChain;

    VulkanDevice::SwapChainSupportDetails swapChainSupport = getSwapChainSupportDetails( surface );

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
    VkPresentModeKHR presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
    extent = chooseSwapExtent( swapChainSupport.capabilities );
    format = surfaceFormat.format;

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }


    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


    VulkanDevice::QueueFamilyIndices indices = _familyIndx;
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


    if (vkCreateSwapchainKHR( _device, &createInfo, nullptr, &swapChain ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create swap chain!" );
    }

    vkGetSwapchainImagesKHR( _device, swapChain, &imageCount, nullptr );
    images.resize( imageCount );
    vkGetSwapchainImagesKHR( _device, swapChain, &imageCount, images.data() );


    return swapChain;
}

std::vector<VkPipeline> VulkanDevice::createPipelines( VkPipelineCache pipelineCache, std::vector<VkGraphicsPipelineCreateInfo> createInfos, VkAllocationCallbacks* pAllocator )
{
    std::vector<VkPipeline> pipelines(createInfos.size());

    if (vkCreateGraphicsPipelines( _device, VK_NULL_HANDLE, createInfos.size(), createInfos.data(), pAllocator, pipelines.data()) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create graphics pipeline!" );
    }

    return pipelines;
}

std::vector<VkCommandBuffer> VulkanDevice::createCommandBuffers( VkCommandPool comandPool, VkCommandBufferLevel level, uint32_t numCommandBuffers )
{
    std::vector<VkCommandBuffer> commandBuffers( numCommandBuffers );

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = comandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = numCommandBuffers;

    if (vkAllocateCommandBuffers( _device, &allocInfo, commandBuffers.data() ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to allocate command buffers!" );
    }

    return commandBuffers;
}

VkCommandPool VulkanDevice::createCommandPool( VkCommandPoolCreateFlags flags, uint32_t familyIndex, VkAllocationCallbacks* pAllocator)
{
    VkCommandPool pool;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = familyIndex;

    if (vkCreateCommandPool( _device, &poolInfo, pAllocator, &pool) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create command pool!" );
    }

    return pool;
}

VkDescriptorPool VulkanDevice::createDescriptorPool( VkDescriptorPoolCreateInfo& createInfo, VkAllocationCallbacks* pAllocator )
{

    VkDescriptorPool descriptor;

    if (vkCreateDescriptorPool( _device, &createInfo, pAllocator, &descriptor ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create descriptor pool!" );
    }

    return descriptor;
}

VkFramebuffer VulkanDevice::createFrameBuffer( VkFramebufferCreateInfo& createInfo, VkAllocationCallbacks* pAllocator )
{
    VkFramebuffer frameBuffer;

    if (vkCreateFramebuffer( _device, &createInfo, pAllocator, &frameBuffer ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create framebuffer!" );
    }
    return frameBuffer;
}

VkSemaphore VulkanDevice::createSemaphore( VkSemaphoreCreateInfo createInfo, VkAllocationCallbacks* pAllocator )
{
    VkSemaphore sem;
    if (vkCreateSemaphore( _device, &createInfo, pAllocator, &sem ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create semaphores!" );
    }
    return sem;
}

VkRenderPass VulkanDevice::createRenderPass( VkRenderPassCreateInfo& createInfo, VkAllocationCallbacks* pAllocator )
{
    VkRenderPass renderPass;

    if (vkCreateRenderPass( _device, &createInfo, pAllocator, &renderPass ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create render pass!" );
    }

    return renderPass;
}

VkFence VulkanDevice::createFence( VkFenceCreateInfo createInfo, VkAllocationCallbacks* pAllocator )
{
    VkFence fence;
    if (vkCreateFence( _device, &createInfo, pAllocator, &fence ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create semaphores!" );
    }
    return fence;
}

void VulkanDevice::createBuffer( VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory )
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    uint32_t indices[] = { _familyIndx.graphicsFamily.value(), _familyIndx.transferFamily.value() };
    bufferInfo.pQueueFamilyIndices = indices;
    bufferInfo.queueFamilyIndexCount = 2;

    if (vkCreateBuffer( _device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create buffer!" );
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( _device, buffer, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, properties );

    if (vkAllocateMemory( _device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to allocate buffer memory!" );
    }

    vkBindBufferMemory( _device, buffer, bufferMemory, 0 );
}

std::vector<VkDescriptorSet> VulkanDevice::createDescriptorSets(std::vector<std::vector<VkWriteDescriptorSet>>& descriptorWrites, const std::vector<VkDescriptorSetLayout>& layouts, VkDescriptorPool descriptorPool )
{
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    //reservamos tanta memoria como sea necesaria para todos los sets de informacion
    descriptorSets.resize( allocInfo.descriptorSetCount );
    if (vkAllocateDescriptorSets( _device, &allocInfo, descriptorSets.data() ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to allocate descriptor sets!" );
    }

    for (size_t i = 0; i < allocInfo.descriptorSetCount; i++) {
        for (auto& writeSet : descriptorWrites[i]) {
            writeSet.dstSet = descriptorSets[i];
        }
        vkUpdateDescriptorSets( _device, static_cast<uint32_t>(descriptorWrites[i].size()), descriptorWrites[i].data(), 0, nullptr);
    }

    return descriptorSets;
}

VkQueue VulkanDevice::getGraphicsQueue()
{
    VkQueue q;
    vkGetDeviceQueue( _device, _familyIndx.graphicsFamily.value(), 0, &q);
    return q;
}

VkQueue VulkanDevice::getPresentQueue()
{
    VkQueue q;
    vkGetDeviceQueue( _device, _familyIndx.presentFamily.value(), 0, &q );
    return q;
}

VkQueue VulkanDevice::getTransferQueue()
{
    VkQueue q;
    vkGetDeviceQueue( _device, _familyIndx.transferFamily.value(), 0, &q );
    return q;
}

VkExtent2D VulkanDevice::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width = 0, height = 0;
        //glfwGetFramebufferSize(_window.window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
        actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

        return actualExtent;
    }
}

VkPresentModeKHR VulkanDevice::chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR VulkanDevice::chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats ) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}