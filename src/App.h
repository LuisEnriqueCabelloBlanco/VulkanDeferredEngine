#pragma once

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/vec4.hpp>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/hash.hpp>



#include<SDL2/SDL_events.h>


#include <chrono>

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <optional>
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <glm/glm.hpp>
#include <array>
#include "VulkanWindow.h"
#include "VulkanDevice.h"
#include "Texture.h"
#include "Utils.h"
#include "BufferObjectsData.h"
#include "Mesh.h"


constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "./pedro.jpeg";

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//struct Vertex {
//    glm::vec3 pos;
//    glm::i8vec3 color;
//    //glm::vec2 texCoord;
//
//    static VkVertexInputBindingDescription getBindingDescription() {
//        VkVertexInputBindingDescription bindingDescription{};
//        bindingDescription.binding = 0;
//        bindingDescription.stride = sizeof(Vertex);
//        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//        return bindingDescription;
//    }
//
//    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
//        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
//        //posicion
//        attributeDescriptions[0].binding = 0;
//        attributeDescriptions[0].location = 0;
//        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
//        attributeDescriptions[0].offset = offsetof(Vertex, pos);
//        //color
//        attributeDescriptions[1].binding = 0;
//        attributeDescriptions[1].location = 1;
//        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
//        attributeDescriptions[1].offset = offsetof(Vertex, color);
//
//        //attributeDescriptions[2].binding = 0;
//        //attributeDescriptions[2].location = 2;
//        //attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
//        //attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
//
//        return attributeDescriptions;
//    }
//
//    bool operator==(const Vertex& other) const {
//        return pos == other.pos && color == other.color /*&& texCoord == other.texCoord*/;
//    }
//};
//
////Hash para los vertices
//namespace std {
//    template<> struct hash<Vertex> {
//        size_t operator()(Vertex const& vertex) const {
//            return ((hash<glm::vec3>()(vertex.pos) ^
//                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) /*^
//                (hash<glm::vec2>()(vertex.texCoord) << 1)*/;
//        }
//    };
//}
//
//struct UniformBufferObject {
//    glm::mat4 model;
//    glm::mat4 view;
//    glm::mat4 proj;
//};

class App
{
public:

    App();

    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

    static void ErrorCallback(int, const char* err_str)
    {
        std::cout << "GLFW Error: " << err_str << std::endl;
    }

private:

    void createSwapChain();

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);


    /*
    * Inicializacion de todos los elementos necesarios para renderizar una imagen en vulkan
    * 
    * 
    * 1.Crear una ventana de SDL
    * 2.Crear la instancia de Vulkan
    * 3.Permitir las capas de validacion
    * 4.Crear la superfice de vulkan a partir de la ventana y la instancia
    * 5.Crear un vulkan device
    * 6.Crear las colas que se vayan a usar durante el renderizado
    * 7.Crear la swapchain (cola de las imagenes que van a ser renderizadas)
    * 8.Creamos los wrapper de vkImage que nos facilitan configurar la forma en la que se acceden
    * 9.Creamos el pase de renderizado que solo toma colores y los envia a pantalla
    * 10.Creamos la pipeline de renderizado
    * 11.Creamos la pool de command buffres (prereserva de espacio para commandbuffers)
    * 12.Creamos los frame buffers 
    * 13.Cargamos el modelo
    * 14.Creamos losbuffers que contendran el modelo
    * 15.Creamos los buffres de commandos (1 por frame al vuelo)
    * 16.Creamos las estructuras de sincronizacion
    */
    void initVulkan() {
        
        _window.init("Proyecto Vulkan",WIDTH,HEIGHT,instance);
        createInstance();
        setupDebugMessenger();
        _window.createSurface( instance );
        _device.init(instance, _window);
        auto indices = _device.getFamilyIndexes();
        graphicsQueue = _device.getGraphicsQueue();
        presentQueue = _device.getPresentQueue();
        trasferQueue = _device.getTransferQueue();

        createSwapChain();
        createImageViews();
        createRenderPass();

        createDescriptorSetLayout();
        createGraphicsPipeline();

        createCommandPool();
        createColorResources();
        createDepthResources();
        createFramebuffers();
        mTexture = new Texture(_device);
        mTexture->loadTexture(TEXTURE_PATH.c_str(), *this);
        
        loadModel();
        //createIndexBuffer();
        createVertexBuffer();
        createUniformBuffers();


        createDescriptorPool();
        createDescriptorSets();

        createCommandBuffers();
        createSyncObjects();
    }


    void createRenderPass();

    //static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void mainLoop() {
        bool running = true;
        while (running)
        {
            SDL_Event ev;
            while (SDL_PollEvent( &ev )) {
                if (ev.type == SDL_QUIT) {
                    running = false;
                }
            }
            //glfwPollEvents();
            
            drawFrame();
            
            //Sleep(100);
        }
        _device.wait();
    }

    void cleanup();

    void createInstance();

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {


        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            // Message is important enough to show
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl<<std::endl;
        }

        return VK_FALSE;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();

    void createImageViews();

    void createGraphicsPipeline();

    void createFramebuffers();
    
    void createCommandPool();

    void createCommandBuffers();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void drawFrame();

    void loadModel();

    void createSyncObjects();

    void recreateSwapChain();

    void cleanupSwapChain();

    void createColorResources();
    public:
    //static void createBuffer(VulkanDevice device, VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    private:
    void updateUniformBuffer(uint32_t currentImage);

    void createDescriptorSetLayout();

    //template <typename t>
    //void createVkBuffer(const std::vector<t>& data, VkBuffer& buffer,
    //    VkDeviceMemory& bufferMemory, VkBufferUsageFlags usage) {
    //    VkDeviceSize bufferSize = sizeof(t) * data.size();

    //    VkBuffer stagingBuffer;
    //    VkDeviceMemory stagingBufferMemory;
    //    _device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //        stagingBuffer, stagingBufferMemory);
    //    
    //    void* dataMap;
    //    _device.mapMemory(stagingBufferMemory, 0, bufferSize, &dataMap);
    //    memcpy(dataMap, data.data(), (size_t)bufferSize);
    //    _device.unmapMemory(stagingBufferMemory);

    //    _device.createBuffer( bufferSize, (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //        buffer, bufferMemory);
    //    copyBuffer(stagingBuffer, buffer, bufferSize);

    //    _device.destroyBuffer( stagingBuffer );
    //    _device.freeMemory( stagingBufferMemory );
    //};

    void createVertexBuffer() {
        //_device.createVkBuffer<Vertex>(vertices, vertexBuffer, vertexBufferMemory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        //_device.createVkBuffer<Vertex>(vertices2, vertexBuffer2, vertexBufferMemory2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    void createIndexBuffer() {
        //_device.createVkBuffer<uint32_t>(indices, indexBuffer, indexBufferMemory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }

    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    void createUniformBuffers();

    void createDescriptorPool();

    void createDepthResources();


    void createDescriptorSets();
    public:
    static uint32_t findMemoryType(VkPhysicalDevice phDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    private:
    static std::vector<char> readFile(const std::string& filename);

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool pool);
    void endSingleTimeCommands(VkCommandPool pool,VkCommandBuffer commandBuffer);
    public:
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void trasitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    private:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue trasferQueue;
    VkSwapchainKHR swapChain;

    std::vector<VkImage> swapChainImages;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

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

    Mesh* mesh;
    Mesh* mesh2;

    //std::vector<VkBuffer> uniformBuffers;
    //std::vector<VkDeviceMemory> uniformBuffersMemory;

    std::vector<Buffer*> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    //std::vector<SwapChainImageContext> swapChainImages;

    VulkanDevice _device;
    VulkanWindow _window;
    Texture* mTexture;
    Texture* depthTexture;
    Texture* msaaTexture;

};
