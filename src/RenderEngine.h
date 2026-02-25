#pragma once
#include<vulkan/vulkan.h>
#include<vector>
#include<VulkanDevice.h>
#include<Texture.h>

class RenderEngine
{
public:
	RenderEngine();

	bool init();
	void close();
    void drawFrame();



private:
    void cleanup();

    void createInstance();

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData ) {


        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            // Message is important enough to show
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl << std::endl;
        }

        return VK_FALSE;
    }

    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo );
    void setupDebugMessenger();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffers();

    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex );

    void createRenderPass();

    void loadModel();

    void createSyncObjects();

    void recreateSwapChain();

    void cleanupSwapChain();

    void createColorResources();

    void updateUniformBuffer( uint32_t currentImage );

    void createDescriptorSetLayout();

    VkFormat findDepthFormat();

    bool hasStencilComponent( VkFormat format );

    void createUniformBuffers();

    void createDescriptorPool();

    void createDepthResources();

    void createDescriptorSets();



private:

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue trasferQueue;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;


    VkCommandPool commandPool;
    VkCommandPool transferPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    //Sinc structures
    std::vector<VkSemaphore> imageAviablesSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    bool _framebufferResized = false;

    uint32_t currentFrame = 0;


    VulkanDevice _device;
    VulkanWindow _window;
    Texture* mTexture;
    Texture* depthTexture;
    Texture* msaaTexture;

    Texture* normalTexture;
    Texture* colorTexture;
    std::vector<VkFramebuffer> colorFramebuffers;
    std::vector<VkFramebuffer> normalFramebuffers;
};

