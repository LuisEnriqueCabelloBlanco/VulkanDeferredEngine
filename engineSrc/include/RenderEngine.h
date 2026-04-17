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
#include "WindowEvent.h"

#include "Camera.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr int MAX_LIGHTS = 100000;

constexpr int MAX_CULL_OBJECTS = 100000;

constexpr int MAX_TEXTURES = 32;

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
    RenderEngine();

    static void ErrorCallback( int, const char* err_str )
    {
        std::cout << "GLFW Error: " << err_str << std::endl;
    }

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

    void drawFrame( std::vector<RenderObject>& objectsArray );

    void wait();

    Camera& getMainCamera() { return _mainCamera; }


    MeshHandle createMesh( const std::string& path );
    MeshHandle createMesh( const std::vector<Vertex>& vertices );
    MeshHandle createMesh( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices );

    void createPointLight(glm::vec3 position, glm::vec3 color, float intensity, float range, bool preload = false);
    void createDirectionalLight( glm::vec3 direction, glm::vec3 color, float intensity, bool preload = false );


    /**
        * @brief carga una textura y devuelve un handle estable
     *
     * @param path
     * @return
     */
        TextureHandle loadTexture( const std::string& path );

        MaterialHandle createMaterial( const MaterialDesc& material );

    void updateGeometryDescriptorSets();

    void updateLightingDescriptorSets();

    void updateLightBuffer();

    void handleWindowEvent( const WindowEvent& event );

    void setMainLight( int index );

private:

    Mesh* getMesh( MeshHandle handle );
    const Mesh* getMesh( MeshHandle handle ) const;
    const MaterialDesc* getMaterial( MaterialHandle handle ) const;

    void createRenderPass();

    void createShadowPass();

    void createInstance(const std::string& appName);

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData ) {


        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            // Message is important enough to show
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            throw std::exception( pCallbackData->pMessage );
        }

        return VK_FALSE;
    }

    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo );
    void setupDebugMessenger();

    void createGraphicsPipeline();

    void createDeferredPipeline();

    void createComputePipeline();

    void createObjectCullPipeline();

    void createShadowPipeline();

    void createFramebuffers();

    void createShadowFrameBuffer();

    void createCommandPool();

    void createCommandBuffers();

    void createComputeCommandPool();

    void createComputeCommandBuffer();

    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex, std::vector<RenderObject>& objectsArray , const std::vector<int>& cullIndex );

    void loadModels();

    void createSyncObjects();

    void recreateSwapChain();

    void cleanupSwapChain();

    void createColorResources();

    void createNormalResources();

    void updateUniformBuffer( uint32_t currentImage, glm::mat4 model );

    void updateComputeDescritorSet();

    void updateShadowDescriptorSet();

    void updateCullDescriptorSet();

    void createLightBuffer();

    void createCullingBufers();

    void createDescriptorSetLayout();

    void createComputeDescriptorSetLayout();

    void createShadowDescriptorSetLayout();

    void createInputAttachmentDescriptorSetLayout();

    void createTextureArrayDescriptorSetLayout();

    void createViewProjectionDescriptorSetLayout();

    void createIndexedObjectsBufferDescriptroSetLayout();

    VkFormat findDepthFormat();

    bool hasStencilComponent( VkFormat format );

    void createUniformBuffers();

    void createDescriptorPool();

    void createDepthResources();

    void createGeometryDescriptorSets();

    void createDeferredDescriptorSets();

    void createComputeDescriptorSets();

    void createObjectCullDescriptorSets();

    void createShadowDesciptorSet();

    void pushModelMatrix( VkCommandBuffer commnadBuffer, glm::mat4 model = glm::mat4( 1 ) );

    void pushTextureIndex( VkCommandBuffer commnadBuffer, const MaterialDesc& material );

    void recordShadowPass( VkCommandBuffer commandBuffer, const std::vector<RenderObject>& objectsArray );

    void recordMainRender( VkCommandBuffer commandBuffer );

    const std::vector<int> cullObjects( const std::vector<RenderObject>& objs, ViewProjectionData& cameraDesc );

    const std::vector<Light> cullLights( const std::vector<Light>& objs );

    void computeCullObects( std::vector<RenderObject>& objectsArray );


public:
    void generateMipmaps( VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels );

private:

    bool AABBFrustrumTest( const AABB& aabb,const glm::mat4& MVP);

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
    VkPipeline noTexPipeline;

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

    VulkanDevice _device;
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

    std::vector<std::unique_ptr<Texture>> _textureArray;
    std::vector<MaterialDesc> _materials;

    std::vector<std::unique_ptr<Mesh>> _meshes;

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

