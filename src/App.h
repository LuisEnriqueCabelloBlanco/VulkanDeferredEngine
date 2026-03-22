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

#include "Camera.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "./untitled.obj";
const std::string MODEL_PATH2 = "./plano.obj";
const std::string TEXTURE_PATH = "./pedro.jpeg";
const std::string TEXTURE2_PATH = "./whitePixel.jpg";
const std::string TEXTURE3_PATH = "./koreano.jpeg";

constexpr int MAX_LIGHTS = 100;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

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
        _window.setDevice( &_device );
        _window.createSwapChain();


        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createDeferredPipeline();

        createCommandPool();
        createColorResources();
        createDepthResources();
        createNormalResources();

        createFramebuffers();

        createCommandBuffers();
        createSyncObjects();

        //cracion de recursos



        mTexture = new Texture(_device);
        mTexture->loadTexture(TEXTURE_PATH);

        renderTexture = new Texture( _device );
        renderTexture->loadTexture( TEXTURE2_PATH);


        loadModel();
        createUniformBuffers();
        createLightBuffer();
        _mainCamera = Camera( glm::vec3( 0, 0, -2.5f ) , glm::vec3( 0, 0, 1.f ), glm::vec3( 0.0f, 1.0f, 0.0f ) ,
                              90.f, _window.getExtent().width / (float)_window.getExtent().height ,0.1f,40.f);

        createDescriptorPool();
        createDescriptorSets();
        createDeferredDescriptorSets();
        updateGeometryDescriptorSets();
        updateLightingDescriptorSets();
    }


    void createRenderPass();

    //static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void mainLoop() {
        bool running = true;
        while (running)
        {
            SDL_Event ev;
            while (SDL_PollEvent( &ev )) {
                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.scancode == SDL_SCANCODE_D) {
                        _mainCamera.translate( glm::vec3(0.f, 0, 1 ) );
                    }
                    if (ev.key.keysym.scancode == SDL_SCANCODE_A) {
                        _mainCamera.translate( glm::vec3( 0.f, 0, -1 ) );
                    }

                    if (ev.key.keysym.scancode == SDL_SCANCODE_W) {
                        _mainCamera.translate( glm::vec3( 1, 0,0 ) );
                    }
                    if (ev.key.keysym.scancode == SDL_SCANCODE_S) {
                        _mainCamera.translate( glm::vec3( -1, 0,0 ) );
                    }

                }

                if (ev.type == SDL_MOUSEMOTION) {
                    _mainCamera.rotateY( ev.motion.xrel * 0.1 );
                }
                if (ev.type == SDL_QUIT) {
                    running = false;
                }
                if (ev.type == SDL_WINDOWEVENT) {
                    _window.handleWindowEvent( ev.window );
                }
            } 
            //glfwPollEvents();
            update();

            drawFrame();

            //renderEngine.drawFrame()
            
            //Sleep(100);
        }
        _device.wait();
    }

    void update();

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

    void createGraphicsPipeline();

    void createDeferredPipeline();

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

    void createNormalResources();

    private:
    void updateUniformBuffer(uint32_t currentImage, glm::mat4 model );


    void createLightBuffer();

    void createDescriptorSetLayout();


    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    void createUniformBuffers();

    void createDescriptorPool();

    void createDepthResources();

    void createDescriptorSets();

    void createDeferredDescriptorSets();

    void updateGeometryDescriptorSets();

    void updateLightingDescriptorSets();

    void pushModelMatrix( VkCommandBuffer commnadBuffer, glm::mat4 model = glm::mat4( 1 ) );

    void pushTextureIndex(VkCommandBuffer commnadBuffer, MaterialData index);
    public:
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
   
    private:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue trasferQueue;

    //VkSwapchainKHR swapChain;
    //VkFormat _swapChainImageFormat;
    //VkExtent2D _swapChainExtent;
    //std::vector<VkImage> swapChainImages;
    //std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkPipeline noTexPipeline;

    VkDescriptorSetLayout deferredDescriptorSetLayout;
    VkPipelineLayout deferredLayout;
    VkPipeline deferredPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    
    VkCommandPool commandPool;
    //VkCommandPool transferPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> lightingDescriptorSets;

    //Sinc structures
    std::vector<VkSemaphore> imageAviablesSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    bool _framebufferResized = false;

    uint32_t currentFrame = 0;

    //lista de enteidades de la escena TODO hacer clase objeto
    std::vector<RenderObject> objects;

    std::vector<Buffer*> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    Buffer* _lightUniformBuffer;
    void* _lightBufferMapped;

    Buffer* _lightBufferSorage;
    std::vector<Light> _lightBuffer;
    void* _lightBufferStorageMapped;

    Camera _mainCamera;

    VulkanDevice _device;
    VulkanWindow _window;
    Texture* mTexture;
    Texture* renderTexture;
    Texture* depthTexture;
    Texture* msaaTexture;

    Texture* normalTexture;
    Texture* colorTexture;
    Texture* posTexture;

    GlobalLighting _lighting;

};
