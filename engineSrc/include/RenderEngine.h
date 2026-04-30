#pragma once

#include <chrono>

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "VulkanInstance.h"
#include "MainRenderPass.h"
#include "VulkanWindow.h"
#include "VulkanDevice.h"
#include "Texture.h"
#include "GBuffer.h"
#include "ShadowPass.h"
#include "RenderPipelines.h"
#include "BufferObjectsData.h"
#include "Mesh.h"
#include "Scene.h"
#include "ResourceLimits.h"
#include "ResourceManager.h"
#include "WindowEvent.h"

#include "DescriptorManager.h"

#include "Camera.h"

#include "CullManager.h"

constexpr int MAX_FRAMES_IN_FLIGHT = ResourceLimits::MAX_FRAMES_IN_FLIGHT;

constexpr int MAX_CULL_OBJECTS = ResourceLimits::MAX_CULL_OBJECTS;

constexpr int MAX_TEXTURES = ResourceLimits::MAX_TEXTURES;

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

    // --- Frame API ----------------------------------------------------------
    void drawFrame();

    // --- Scene y recursos ---------------------------------------------------
    // La escena es la unica fuente de verdad para entidades y luces.
    // La aplicacion crea luces a traves de scene.createLight() y designa
    // la main light con scene.setMainLight(). RenderEngine consulta la escena
    // cada frame internamente; no expone ninguna API propia de luces.
    Scene& getScene() { return _scene; }
    ResourceManager& getResourceManager() { return _resources; }

    // --- Camara y eventos ---------------------------------------------------
    Camera& getMainCamera() { return _mainCamera; }
    void    handleWindowEvent(const WindowEvent& event);

    // --- Utilidades ---------------------------------------------------------
    void generateMipmaps(VkImage image, VkFormat imageFormat,
        int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    static void ErrorCallback(int, const char* err_str) {
        std::cout << "GLFW Error: " << err_str << std::endl;
    }

private:
    // --- Framebuffer y command infrastructure -------------------------------
    void createCommandPool();
    void createCommandBuffers();
    void createComputeCommandPool();
    void createComputeCommandBuffer();
    void createSyncObjects();
    void recreateSwapChain();
    void performSwapChainRecreation();

    // --- GPU resources y descriptor sets ---------------------------------
    void createUniformBuffers();
    void createLightBuffer();
    void createCullingBuffers();

    // --- Updates de descriptors -------------------------------------------
    // Llamar en init() o cuando cambien buffers/texturas/swapchain.
    void updateComputeDescriptorSet();   // tras crear buffers de culling / lighting
    void updateShadowDescriptorSet();    // tras crear el buffer de VP de la main light
    void updateCullDescriptorSet();      // tras crear buffers de culling
    void updateGeometryDescriptors();     // tras cargar/liberar texturas
    void updateLightingDescriptors();     // tras resize / recrear GBuffer o shadow map

    // --- Runtime updates ----------------------------------------------------
    void updateUniformBuffer(uint32_t currentImage, glm::mat4 model);

    // Vuelca la light queue de la escena al storage buffer de GPU.
    // Llamado cada frame desde drawFrame(), no desde la API publica.
    void uploadLightBuffer(const std::vector<LightObject>& lights);

    // --- Recording y culling ------------------------------------------------
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                             const std::vector<RenderObject>& objectsArray,
                             const std::vector<int>& cameraIndex,
                             const std::vector<int>& shadowIndex);
    void recordShadowPass(VkCommandBuffer commandBuffer,
                          const std::vector<RenderObject>& objectsArray,
                          const std::vector<int>& cullIndex);
    void recordMainRender(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         const std::vector<RenderObject>& objectsArray,
                         const std::vector<int>& cullIndex);
    void pushModelMatrix(VkCommandBuffer commandBuffer, glm::mat4 model = glm::mat4(1));
    void pushTextureIndex(VkCommandBuffer commandBuffer, const MaterialData& material);


    // --- Misc ---------------------------------------------------------------
    bool hasStencilComponent(VkFormat format);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

    // --- Vulkan handles -----------------------------------------------------
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;

    RenderPipelines _pipelines;

    VkCommandPool computeComandPool;
    VkCommandBuffer computeCommandBuffer;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAviablesSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    bool _swapChainRecreationPending = false;
    uint32_t currentFrame = 0;

    // --- GPU buffers --------------------------------------------------------
    std::vector<Buffer*> uniformBuffers;
    std::vector<void*>   uniformBuffersMapped;

    Buffer* _lightUniformBuffer = nullptr;   // GlobalLighting UBO (eyePos, ambient)
    void* _lightBufferMapped = nullptr;

    Buffer* _lightBufferStorage = nullptr;   // Storage buffer con todos los LightObjects
    void* _lightBufferStorageMapped = nullptr;

    Buffer* _lightIndexStorage;
    Buffer* _AABBModelStorage;
    void* _AABBModelStorageMapped;

    Buffer* _lightCulledObjectIndex;
    int* _lightCulledObjectIndexMapped;

    Buffer* _cameraCulledObjectIndex;
    int* _cameraCulledObjectIndexMapped;

    // Shadow pass: VP desde el punto de vista de la main light.
    // Se calcula en updateUniformBuffer() consultando _scene.tryGetMainLight().
    Buffer* _mainLightVPBuffer = nullptr;   // UBO ViewProjectionData
    ViewProjectionData* _mainLightVPMapped = nullptr;

    // --- Engine objects -----------------------------------------------------
    Camera _mainCamera;
    Scene _scene;
    VulkanInstance _vulkanInstance;
    MainRenderPass _mainRenderPass;
    VulkanDevice _device;
    ResourceManager _resources;
    VulkanWindow _window;
    CullManager _culler;
    GBuffer _gbuffer;
    ShadowPass _shadowPass;

    GlobalLighting _lighting;

    DescriptorManager _descriptors;


};

