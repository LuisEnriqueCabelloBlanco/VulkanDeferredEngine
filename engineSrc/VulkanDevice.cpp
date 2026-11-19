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

#ifdef _DEBUG
    std::cout << "\nCreating Device\n"<<std::endl;
#endif // _DEBUG

    _familyIndx = findQueueFamilies(_physicalDevice,*_window);

#ifdef _DEBUG
    std::cout << "Family Indexes:\n"
        "\tGraphics Index: "<< _familyIndx.graphicsFamily.value() << "\n"<<
        "\tCompute Index: " << _familyIndx.computeFamily.value() << "\n" <<
        "\tPresent Index: " << _familyIndx.presentFamily.value() << "\n" <<
        "\tTransferOnly Index: " << _familyIndx.transferOnlyFamily.value() << std::endl;
#endif // _DEBUG


    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { _familyIndx.graphicsFamily.value(),_familyIndx.presentFamily.value(),_familyIndx.transferOnlyFamily.value()};


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

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{};
    descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

    // Enable non-uniform indexing
    descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;
    descriptor_indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptor_indexing_features.descriptorBindingPartiallyBound = VK_TRUE;

    descriptor_indexing_features.pNext;

    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    // set the desired features here:
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;
    features13.pNext = &descriptor_indexing_features;


    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.features = deviceFeatures;
    features2.pNext = &features13;


    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = NULL;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = &features2;

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
}

void VulkanDevice::init(VkInstance instance, VulkanWindow& window)
{
    _vkInstance = instance;
    _window = &window;
    pickPhysicalDevice();
    createDevice();

    //estructuras para la copia de datos (seguramente se mueva a un gestor a parte
    _transferQueue = getTransferQueue();
    _transferPool = createCommandPool( VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, _familyIndx.transferOnlyFamily.value() );
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
    destroyCommandPool( _transferPool );

    vkDestroyDevice(_device, nullptr);
}

VkImage VulkanDevice::createImage( uint32_t width, uint32_t height, uint32_t mipLvl, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
    VkImageLayout layout, uint32_t layers, VkImageCreateFlags flags)
{
    VkImage img;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLvl;
    imageInfo.arrayLayers = layers;
    /*Vulkan supports many possible image formats,
    but we should use the same format for the texels as the pixels in the buffer,
    otherwise the copy operation will fail.*/
    imageInfo.format = format;
    imageInfo.initialLayout = layout;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = numSamples;
    imageInfo.flags = flags; // Optional
    if (vkCreateImage(_device, &imageInfo, nullptr, &img ) != VK_SUCCESS) {
        throw std::runtime_error( "failed to create image!" );
    }
    return img;
}

VkImageView VulkanDevice::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageViewType viewType, uint32_t layerCount)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;

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

#ifdef _DEBUG
    std::cout << "Available Devices: " << deviceCount<< "\n";
#endif // _DEBUG


    uint32_t maxMemory = 0;

    VkPhysicalDeviceProperties2 prop;

    for (const auto& device : devices) {

        VkPhysicalDeviceProperties2 candidateProp = VkPhysicalDeviceProperties2();

        candidateProp.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkGetPhysicalDeviceProperties2( device, &candidateProp );

#ifdef _DEBUG
            std::cout<<"\Evaluating: "<< candidateProp.properties.deviceName << "\n";
#endif // _DEBUG

        if (isDeviceSuitable(device, candidateProp.properties)){

            VkPhysicalDeviceMemoryProperties2 memProps = VkPhysicalDeviceMemoryProperties2();
            memProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
            vkGetPhysicalDeviceMemoryProperties2(device, &memProps);
            VkDeviceSize maxDeviceMemory =0;

            for (int i = 0; i < memProps.memoryProperties.memoryHeapCount; i++) {
                maxDeviceMemory += memProps.memoryProperties.memoryHeaps[i].size;
            }

            if (maxDeviceMemory > maxMemory) {
                maxMemory = maxDeviceMemory;
                _physicalDevice = device;
                msaaSamples = getMaxUsableSampleCount();
            }

        }
    }

    if (_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}


bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkPhysicalDeviceProperties properties) {
    QueueFamilyIndices indices = findQueueFamilies(device, *_window);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    //if (extensionsSupported) {
    //    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    //    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    //}

    uint32_t version;
    vkEnumerateInstanceVersion(&version);

    return indices.isComplete() && extensionsSupported&&VK_VERSION_MINOR(properties.apiVersion)<= VK_VERSION_MINOR(version)/* && swapChainAdequate*/;
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionsCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);

    std::vector<VkExtensionProperties> aviableExtensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, aviableExtensions.data());

    //enumeracion de las extensiondes disponibles
#ifdef _DEBUG
    //for (auto& ext : aviableExtensions) {
    //    std::cout << ext.extensionName << "\n";
    //}
#endif // _DEBUG

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : aviableExtensions) {

#ifdef _DEBUG
        if (requiredExtensions.count(extension.extensionName)) {
            std::cout << extension.extensionName << " will be activated" << std::endl;
        }
#endif // _DEBUG

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

VkFormat VulkanDevice::findDepthFormat()
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device, VulkanWindow& window)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, nullptr);


#ifdef _DEBUG
    std::cout << "Existing families in this device: " << queueFamilyCount<<"\n";
#endif // _DEBUG


    VkQueueFamilyProperties2 prop = VkQueueFamilyProperties2();
    prop.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;

    std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamilyCount,prop);
    vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, queueFamilies.data());

    //Recorre y mira a que famialia se le puede asignar el dispositivo
    // Logic to find queue family indices to populate struct with
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {


        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, window.getSurface(), &presentSupport);

        indices.fillIndices(i,queueFamily.queueFamilyProperties.queueFlags,presentSupport);

#ifdef _DEBUG
        std::cout << queueFamily.queueFamilyProperties.queueCount << " queues in this family: \t" <<
            " graphics:"<< ((queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)!=0)<<
            " compute:"<< ((queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)!=0) << 
            " transfer:"<< ((queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)!=0) <<
            " present:"<<presentSupport << std::endl;
#endif // _DEBUG

        i++;
    }

    if (!indices.isComplete()) {
        indices.transferOnlyFamily = indices.graphicsFamily;
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

std::vector<VkPipeline> VulkanDevice::createcomputePipelines( VkPipelineCache pipelineCache, std::vector<VkComputePipelineCreateInfo> createInfos, VkAllocationCallbacks* pAllocator )
{
    std::vector<VkPipeline> pipelines( createInfos.size() );

    if (vkCreateComputePipelines( _device, VK_NULL_HANDLE, createInfos.size(), createInfos.data(), pAllocator, pipelines.data() ) != VK_SUCCESS) {
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

/**
 * @brief funcion que solo debe ser usada desde Buffer
 * @param size 
 * @param usage 
 * @param properties 
 * @param buffer 
 * @param bufferMemory 
 */
void VulkanDevice::createBuffer( VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, 
                                 VkBuffer& buffer, VkDeviceMemory& bufferMemory, std::vector<uint32_t> familyIndex)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

    uint32_t indices[] = { _familyIndx.graphicsFamily.value(), _familyIndx.transferOnlyFamily.value() };
    if (familyIndex.size() == 0) {
        bufferInfo.pQueueFamilyIndices = indices;
        bufferInfo.queueFamilyIndexCount = 2;
        //std::cout << "No se pasaron indices"<<"\n";
    }
    else {
        bufferInfo.pQueueFamilyIndices = familyIndex.data();
        bufferInfo.queueFamilyIndexCount = familyIndex.size();
        //std::cout << "Se pasaron indices"<<"\n";
    }

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

Buffer* VulkanDevice::createBuffer( VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties )
{
    return new Buffer(size, usage, properties,this);
}

std::vector<VkDescriptorSet> VulkanDevice::createDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, VkDescriptorPool descriptorPool, void* pNext )
{
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();
    allocInfo.pNext = pNext;

    //reservamos tanta memoria como sea necesaria para todos los sets de informacion
    descriptorSets.resize( layouts.size() );
    VkResult res =vkAllocateDescriptorSets( _device, &allocInfo, descriptorSets.data() );
    if (res!= VK_SUCCESS) {
        
        if (res == VkResult::VK_ERROR_OUT_OF_POOL_MEMORY) {
            std::cout << "No hay mas pool";
        }

        if (res == VkResult::VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            std::cout << "No hay mas memoria en el device";
        }
        throw std::runtime_error( "failed to allocate descriptor sets!" );
    }

    return descriptorSets;
}

void VulkanDevice::updateDescriptorSet(const std::vector<VkWriteDescriptorSet>& writes )
{
    vkUpdateDescriptorSets( _device, writes.size(), writes.data(), 0, nullptr );
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
    vkGetDeviceQueue( _device, _familyIndx.transferOnlyFamily.value(), 0, &q );
    return q;
}

VkQueue VulkanDevice::getComputeQueue()
{
    VkQueue q;
    vkGetDeviceQueue( _device, _familyIndx.computeFamily.value(), 0, &q );
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

void VulkanDevice::copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,VkDeviceSize offSetSrc, VkDeviceSize offSetDst )
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands( _transferPool );

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = offSetSrc; // Optional
    copyRegion.dstOffset = offSetDst; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

    endSingleTimeCommands( _transferPool, commandBuffer , _transferQueue);
}


void VulkanDevice::copyBufferToImage( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height ) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(_transferPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(_transferPool,commandBuffer,_transferQueue);
}

//TODO move to class
VkAccessFlags2 VulkanDevice::getAccesFlag(VkImageLayout layout) {
    switch (layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return 0;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_GENERAL:
            assert(false &&
                "Don't know how to get a meaningful VkAccessFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
            return 0;
        default:
            assert(false);
            return 0;
    }
}

VkPipelineStageFlags2 VulkanDevice::getStage(VkImageLayout layout) {

    switch (layout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_GENERAL:
            assert(
                false &&
                "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
            return 0;
        default:
            assert(false);
            return 0;
    }

}

void VulkanDevice::singleTimeTransitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels )
{
    //TODO perfilar el metodo
    VkCommandPool usedPool = _transferPool;
    VkQueue usedQueue = _transferQueue;

    VkImageMemoryBarrier2 barrier;
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.pNext = NULL;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || 
        newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
        oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        //si tiene componente de stencil lo marcamos
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags2 sourceStage;
    VkPipelineStageFlags2 destinationStage;

    barrier.srcAccessMask = getAccesFlag(oldLayout);
    barrier.dstAccessMask = getAccesFlag(newLayout);
    barrier.srcStageMask = getStage(oldLayout);
    barrier.dstStageMask = getStage(newLayout);


    //si es a operacion de copia de la imagen
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        //barrier.srcAccessMask = VK_ACCESS_NONE;
        //barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        usedPool = createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, _familyIndx.graphicsFamily.value());;
        usedQueue = getGraphicsQueue();

        //sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        //destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    //si es la transicion de imagen a sample de shader
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        //barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        usedPool = createCommandPool( VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, _familyIndx.graphicsFamily.value() );;
        usedQueue = getGraphicsQueue();

        //sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        //destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    //si es la transicion de imagen para crear la stencil y depth
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        //barrier.srcAccessMask = VK_ACCESS_NONE;
        //barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        //sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        //destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    //Transicion en la inicializacion de la textura de sombras
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        usedPool = createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, _familyIndx.graphicsFamily.value());;
        usedQueue = getGraphicsQueue();
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL) {

        //barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        //barrier.dstStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_;

        usedPool = createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, _familyIndx.graphicsFamily.value());;
        usedQueue = getGraphicsQueue();
    }
    else if(barrier.srcAccessMask == 0 && barrier.dstAccessMask==0 && barrier.srcStageMask ==0 && barrier.dstStageMask==0){
        throw std::invalid_argument( "unsupported layout transition!" );
    }


    VkCommandBuffer commandBuffer = beginSingleTimeCommands( usedPool );

    VkDependencyInfo info;
    info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    info.pImageMemoryBarriers = &barrier;
    info.imageMemoryBarrierCount = 1;
    info.pBufferMemoryBarriers = nullptr;
    info.bufferMemoryBarrierCount = 0;
    info.pMemoryBarriers = nullptr;
    info.memoryBarrierCount = 0;
    info.dependencyFlags = NULL;
    info.pNext = nullptr;

    vkCmdPipelineBarrier2(commandBuffer, &info);


    endSingleTimeCommands( usedPool, commandBuffer, usedQueue);

    //if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    //    || (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)) {
    //    destroyCommandPool( usedPool );
    //}

    if (usedPool != _transferPool) {
        destroyCommandPool(usedPool);
    }

}

void VulkanDevice::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{

    VkImageMemoryBarrier2 barrier;
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.pNext = NULL;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
        oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        //si tiene componente de stencil lo marcamos
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags2 sourceStage;
    VkPipelineStageFlags2 destinationStage;

    barrier.srcAccessMask = getAccesFlag(oldLayout);
    barrier.dstAccessMask = getAccesFlag(newLayout);
    barrier.srcStageMask = getStage(oldLayout);
    barrier.dstStageMask = getStage(newLayout);

    VkDependencyInfo info;
    info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    info.pImageMemoryBarriers = &barrier;
    info.imageMemoryBarrierCount = 1;
    info.pBufferMemoryBarriers = nullptr;
    info.bufferMemoryBarrierCount = 0;
    info.pMemoryBarriers = nullptr;
    info.memoryBarrierCount = 0;
    info.dependencyFlags = NULL;
    info.pNext = nullptr;

    vkCmdPipelineBarrier2(commandBuffer, &info);

}
