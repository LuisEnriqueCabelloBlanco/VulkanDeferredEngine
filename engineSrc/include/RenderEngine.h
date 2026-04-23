#pragma once

#include <chrono>

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "VulkanWindow.h"
#include "VulkanDevice.h"
#include "Texture.h"
#include "BufferObjectsData.h"
#include "Mesh.h"
#include "Scene.h"
#include "ResourceLimits.h"
#include "ResourceManager.h"
#include "WindowEvent.h"

#include "Camera.h"

constexpr int MAX_FRAMES_IN_FLIGHT = ResourceLimits::MAX_FRAMES_IN_FLIGHT;

constexpr int MAX_LIGHTS = ResourceLimits::MAX_LIGHTS;

constexpr int MAX_CULL_OBJECTS = ResourceLimits::MAX_CULL_OBJECTS;

constexpr int MAX_TEXTURES = ResourceLimits::MAX_TEXTURES;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

class RenderEngine
{
public:
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

    void init(const std::string& appName);
    void cleanup();
    void wait();
    RenderEngine();

    // Frame API
    void drawFrame();

    // Scene API
    Scene& getSceneInternal() { return _scene; }
    ResourceManager& getResourceManagerInternal() { return _resources; }

    // Camera and events
    Camera& getMainCamera() { return _mainCamera; }
    void handleWindowEvent( const WindowEvent& event );

    // Lighting API
    void createPointLight(glm::vec3 position, glm::vec3 color, float intensity, float range, bool preload = false);
    void createDirectionalLight( glm::vec3 direction, glm::vec3 color, float intensity, bool preload = false );
    void setMainLight( int index );
    void updateLightBuffer();

    // Explicit descriptor updates
    void updateGeometryDescriptorSets();
    void updateLightingDescriptorSets();

    // Utility
    void generateMipmaps( VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels );

    static void ErrorCallback( int, const char* err_str )
    {
        std::cout << "GLFW Error: " << err_str << std::endl;
    }

private:

    // Vulkan bootstrap
    void createInstance(const std::string& appName);
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo );
    void setupDebugMessenger();

    // Render pass and pipeline creation
    void createRenderPass();
    void createShadowPass();
    void createGraphicsPipeline();
    void createDeferredPipeline();
    void createComputePipeline();
    void createObjectCullPipeline();
    void createShadowPipeline();

    // Framebuffer and command infrastructure
    void createFramebuffers();
    void createShadowFrameBuffer();
    void createCommandPool();
    void createCommandBuffers();
    void createComputeCommandPool();
    void createComputeCommandBuffer();
    void createSyncObjects();
    void recreateSwapChain();
    void cleanupSwapChain();

    // GPU resources and descriptor layouts
    void createColorResources();
    void createNormalResources();
    void createDepthResources();
    void createUniformBuffers();
    void createLightBuffer();
    void createCullingBuffers();
    void createDescriptorPool();

    void createDescriptorSetLayout();
    void createComputeDescriptorSetLayout();
    void createShadowDescriptorSetLayout();
    void createInputAttachmentDescriptorSetLayout();
    void createTextureArrayDescriptorSetLayout();
    void createViewProjectionDescriptorSetLayout();
    void createIndexedObjectsBufferDescriptorSetLayout();

    void createGeometryDescriptorSets();
    void createDeferredDescriptorSets();
    void createComputeDescriptorSets();
    void createObjectCullDescriptorSets();
    void createShadowDescriptorSet();

    // Runtime updates
    void updateUniformBuffer( uint32_t currentImage, glm::mat4 model );
    void updateComputeDescriptorSet();
    void updateShadowDescriptorSet();
    void updateCullDescriptorSet();

    // Recording and culling
    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<RenderObject>& objectsArray , const std::vector<int>& cullIndex );
    void recordShadowPass( VkCommandBuffer commandBuffer, const std::vector<RenderObject>& objectsArray );
    void recordMainRender( VkCommandBuffer commandBuffer );
    void pushModelMatrix( VkCommandBuffer commandBuffer, glm::mat4 model = glm::mat4( 1 ) );
    void pushTextureIndex( VkCommandBuffer commandBuffer, const MaterialDesc& material );

    const std::vector<int> cullObjects( const std::vector<RenderObject>& objs, ViewProjectionData& cameraDesc );
    const std::vector<Light> cullLights( const std::vector<Light>& objs );
    void computeCullObjects( std::vector<RenderObject>& objectsArray );

    // Misc
    void loadModels();
    VkFormat findDepthFormat();
    bool hasStencilComponent( VkFormat format );
    bool AABBFrustrumTest( const AABB& aabb,const glm::mat4& MVP);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData ) {


        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            // Message is important enough to show
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }

        return VK_FALSE;
    }

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkRenderPass shadowPass;
    VkPipelineLayout _shadowPipelineLayout;
    VkPipeline _shadowPipeline;

    VkPipelineLayout _computeLayout;
    VkPipeline _computePipeline;

    VkPipelineLayout _objectCullingPipelineLayout;
    VkPipeline _objectCullingPipeline;

    VkPipelineLayout deferredLayout;
    VkPipeline deferredPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkFramebuffer _shadowFrameBuffer;

    VkCommandPool computeComandPool;
    VkCommandBuffer computeCommandBuffer;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    //Sinc structures
    std::vector<VkSemaphore> imageAviablesSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    bool _framebufferResized = false;

    uint32_t currentFrame = 0;

    std::vector<Buffer*> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    Buffer* _lightUniformBuffer;
    void* _lightBufferMapped;

    Buffer* _lightBufferSorage;
    std::vector<Light> _lightBuffer;
    void* _lightBufferStorageMapped;

    Buffer* _lightIndexStorage;

    Buffer* _AABBModelStorage;
    void* _AABBModelStorageMapped;

    Buffer* _lightCulledObjectIndex;
    int* _lightCulledObjectIndexMapped;

    Buffer* _cameraCulledObjectIndex;
    int* _cameraCulledObjectIndexMapped;

    Camera _mainCamera;
    Scene _scene;

    VulkanDevice _device;
    ResourceManager _resources;
    VulkanWindow _window;

    Texture* msaaTexture;

    Texture* depthTexture;
    Texture* normalTexture;
    Texture* colorTexture;
    Texture* posTexture;

    Texture* shadowMap;
    int _mainLightIndex = -1;
    Buffer* mainLightData;
    ViewProjectionData* lightCameraUBO;

    GlobalLighting _lighting;

    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout _inputAttachmentsDescriptorSetLayout;
    VkDescriptorSetLayout _textureArrayDescriptorSetLayout;
    VkDescriptorSetLayout _viewProjectionDescriptorSetLayout;
    VkDescriptorSetLayout _indexedObjectsBufferDescriptroSetLayout;
    VkDescriptorSetLayout _computeDescriptorSetLayout;
    VkDescriptorSetLayout deferredDescriptorSetLayout;

    std::vector<VkDescriptorSet> _computeDescriptorSet;
    std::vector<VkDescriptorSet> _inputAttachemntsDescriptorSet;
    std::vector<VkDescriptorSet> _cameraDescriptorSet;
    std::vector<VkDescriptorSet> _globalLightingDescriptorSets;

    VkDescriptorSet _textureArrayDescriptorSet;
    VkDescriptorSet _mainLightDescriptorSet;
    VkDescriptorSet _lightsDataBufferDescriptroSet;
    VkDescriptorSet _cameraCullDescriptorSet;
    VkDescriptorSet _lightCullDescriptorSet;

};

