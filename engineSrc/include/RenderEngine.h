#pragma once

#include <chrono>
#include <iostream>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "VulkanInstance.h"
#include "VulkanWindow.h"
#include "VulkanDevice.h"
#include "MainRenderPass.h"
#include "GBuffer.h"
#include "ShadowPass.h"
#include "RenderPipelines.h"
#include "DescriptorManager.h"
#include "GPUBufferManager.h"
#include "CullManager.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "Scene.h"
#include "BufferObjectsData.h"
#include "Mesh.h"
#include "Texture.h"
#include "ResourceLimits.h"
#include "WindowEvent.h"

constexpr int MAX_FRAMES_IN_FLIGHT = ResourceLimits::MAX_FRAMES_IN_FLIGHT;
constexpr int MAX_CULL_OBJECTS     = ResourceLimits::MAX_CULL_OBJECTS;
constexpr int MAX_TEXTURES         = ResourceLimits::MAX_TEXTURES;

// =============================================================================
// RenderEngine
// =============================================================================
/*
  Orquestador principal del sistema de renderizado deferred basado en Vulkan.

  RESPONSABILIDADES
  -----------------
  - Coordinar el ciclo de vida de todos los subsistemas (init / cleanup).
  - Ejecutar el loop de frame: culling, actualizacion de buffers GPU,
    grabacion del command buffer y presentacion.
  - Gestionar la recreacion de la swapchain ante eventos de resize.
  - Exponer una API minima al codigo de aplicacion: escena, recursos y camara.

  LO QUE NO HACE DIRECTAMENTE
  ----------------------------
  Cada responsabilidad especifica esta delegada a un subsistema propio:
    VulkanInstance    — creacion del VkInstance y debug messenger.
    VulkanWindow      — ventana SDL, surface y swapchain.
    VulkanDevice      — dispositivo fisico/logico y colas.
    MainRenderPass    — render pass deferred de dos subpasses.
    ShadowPass        — render pass, shadow map y framebuffer de sombras.
    GBuffer           — texturas y framebuffers del geometry buffer.
    RenderPipelines   — descriptor set layouts, pipeline layouts y pipelines.
    DescriptorManager — pool y sets de descriptores.
    GPUBufferManager  — buffers GPU de camara, luces y culling.
    CullManager       — frustum culling CPU contra multiples frustums.
    ResourceManager   — carga y gestion de meshes, texturas y materiales.
    Scene             — entidades y luces de la escena.

  CICLO DE VIDA
  -------------
    RenderEngine engine;
    engine.init( "MyApp" );
    while ( running ) engine.drawFrame();
    engine.cleanup();

  RESIZE
  ------
  RenderEngine detecta el resize a traves de handleWindowEvent(). El flag
  _swapChainRecreationPending se activa y la recreacion se ejecuta al inicio
  del siguiente drawFrame(), tras esperar a que la GPU este ociosa.
  El render pass principal y las pipelines solo se recrean si el formato de
  la swapchain cambia, lo cual es infrecuente.
*/
class RenderEngine
{
public:
    RenderEngine();

    // Inicializa todos los subsistemas en orden de dependencia.
    void init( const std::string& appName );

    // Espera a que la GPU termine y destruye todos los recursos en orden inverso.
    void cleanup();

    // Bloquea hasta que el device este ocioso. Util antes de liberar recursos.
    void wait();

    // -------------------------------------------------------------------------
    // Frame API
    // -------------------------------------------------------------------------

    // Ejecuta un frame completo: culling, update de buffers, grabacion
    // del command buffer y presentacion. Si hay un resize pendiente,
    // lo resuelve y retorna sin renderizar ese frame.
    void drawFrame();

    // -------------------------------------------------------------------------
    // Acceso a subsistemas para el codigo de aplicacion
    // -------------------------------------------------------------------------

    // La escena es la unica fuente de verdad para entidades y luces.
    // Crear luces con scene.createLight() y designar la main light
    // con scene.setMainLight(). RenderEngine consulta la escena cada frame.
    Scene&           getScene()           { return _scene;     }
    ResourceManager& getResourceManager() { return _resources; }

    // Propaga eventos de ventana (resize, etc.) al engine y a VulkanWindow.
    void handleWindowEvent( const WindowEvent& event );

    // -------------------------------------------------------------------------
    // Utilidades
    // -------------------------------------------------------------------------

    void generateMipmaps( VkImage image, VkFormat imageFormat,
                          int32_t texWidth, int32_t texHeight,
                          uint32_t mipLevels );

    static void ErrorCallback( int, const char* err_str ) {
        std::cout << "GLFW Error: " << err_str << std::endl;
    }

private:

    // =========================================================================
    // Inicializacion de infraestructura de comandos
    // =========================================================================

    void createCommandPool();
    void createCommandBuffers();

    // Command pool y buffer dedicados al compute (light culling).
    // Actualmente en desuso activo pero reservados para el compute pipeline.
    void createComputeCommandPool();
    void createComputeCommandBuffer();

    // Crea semaforos y fences de sincronizacion por frame en vuelo.
    void createSyncObjects();

    // =========================================================================
    // Gestion de swapchain y resize
    // =========================================================================

    // Destruye y recrea la swapchain, el GBuffer y, si el formato cambia,
    // el render pass principal y las pipelines.
    void recreateSwapChain();

    // Llama a recreateSwapChain() y actualiza los descriptor sets afectados.
    // Se ejecuta al inicio del siguiente frame tras detectar un resize.
    void performSwapChainRecreation();

    // =========================================================================
    // Actualizacion de descriptor sets
    // =========================================================================
    // Cada metodo delega en DescriptorManager con los datos exactos necesarios.
    // Se llaman en init() tras crear los buffers, y en resize cuando los
    // recursos subyacentes cambian.

    void updateComputeDescriptorSet();  // GlobalLighting UBO para light culling
    void updateShadowDescriptorSet();   // VP buffer de la main light
    void updateCullDescriptorSet();     // Buffers de culling (AABB, indices)
    void updateGeometryDescriptors();   // VP UBO por frame + array de texturas
    void updateLightingDescriptors();   // Input attachments + shadow map + lighting UBO

    // =========================================================================
    // Grabacion del command buffer
    // =========================================================================

    // Punto de entrada: graba el compute de light culling, el shadow pass
    // y el main render pass en el command buffer del frame actual.
    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex,
                              const std::vector<RenderObject>& objectsArray,
                              const std::vector<int>& cameraIndex,
                              const std::vector<int>& shadowIndex );

    // Graba el render pass de sombras desde el punto de vista de la main light.
    // No hace nada si no hay main light designada en la escena.
    void recordShadowPass( VkCommandBuffer commandBuffer,
                           const std::vector<RenderObject>& objectsArray,
                           const std::vector<int>& cullIndex );

    // Graba el render pass principal: subpass 0 (geometry) y subpass 1 (lighting).
    void recordMainRender( VkCommandBuffer commandBuffer, uint32_t imageIndex,
                           const std::vector<RenderObject>& objectsArray,
                           const std::vector<int>& cullIndex );

    // Push constants: model matrix (vertex) y material index (fragment).
    void pushModelMatrix(  VkCommandBuffer commandBuffer,
                           glm::mat4 model = glm::mat4( 1 ) );
    void pushTextureIndex( VkCommandBuffer commandBuffer,
                           const MaterialData& material );

    // =========================================================================
    // Miscelanea
    // =========================================================================

    bool hasStencilComponent( VkFormat format );

    // Callback de validacion de Vulkan. Solo imprime mensajes de WARNING o superior.
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT   messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT          messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData )
    {
        if ( messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << "\n";
        }
        return VK_FALSE;
    }

    // =========================================================================
    // Resolucion inicial de la ventana
    // =========================================================================

    const uint32_t WIDTH  = 800;
    const uint32_t HEIGHT = 600;

    // =========================================================================
    // Infraestructura Vulkan de bajo nivel
    // =========================================================================

    // Colas obtenidas del device tras su inicializacion.
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;

    // MSAA: actualmente en 1 sample (sin MSAA). Preparado para activarse.
    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    // Command pool y buffers graficos (uno por frame en vuelo).
    VkCommandPool                commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // Command pool y buffer dedicados al compute (reservados, no activos).
    VkCommandPool   computeComandPool;
    VkCommandBuffer computeCommandBuffer;

    // Primitivas de sincronizacion por frame en vuelo.
    std::vector<VkSemaphore> imageAviablesSemaphores;  // imagen de swapchain disponible
    std::vector<VkSemaphore> renderFinishedSemaphores; // frame terminado, listo para presentar
    std::vector<VkFence>     inFlightFences;            // CPU espera a que la GPU termine el frame

    // Indice del frame actual dentro del rango [0, MAX_FRAMES_IN_FLIGHT).
    uint32_t currentFrame = 0;

    // Flag activado por handleWindowEvent(Resized). La recreacion se ejecuta
    // al inicio del siguiente drawFrame().
    bool _swapChainRecreationPending = false;

    // =========================================================================
    // Subsistemas — cada uno posee su propio ciclo de vida
    // =========================================================================

    // Bootstrap de Vulkan: VkInstance y debug messenger.
    VulkanInstance _vulkanInstance;

    // Ventana SDL, VkSurfaceKHR y swapchain.
    VulkanWindow   _window;

    // Dispositivo fisico/logico, colas y operaciones de bajo nivel.
    VulkanDevice   _device;

    // Render pass principal: subpass 0 (geometry) + subpass 1 (lighting deferred).
    MainRenderPass _mainRenderPass;

    // Shadow pass: render pass propio, shadow map y framebuffer.
    ShadowPass     _shadowPass;

    // Geometry buffer: texturas de albedo, normales, posicion, profundidad
    // y framebuffers vinculados a la swapchain.
    GBuffer        _gbuffer;

    // Descriptor set layouts, pipeline layouts y pipelines compiladas.
    RenderPipelines _pipelines;

    // Pool de descriptores y todos los VkDescriptorSets del motor.
    DescriptorManager _descriptors;

    // Buffers GPU: VP de camara, GlobalLighting, VP de la main light,
    // light storage y buffers de culling.
    GPUBufferManager _buffers;

    // Frustum culling CPU contra multiples frustums en una sola pasada.
    CullManager _culler;

    // Carga y ciclo de vida de meshes, texturas y materiales.
    ResourceManager _resources;

    // Entidades renderizables y luces de la escena.
    Scene _scene;


    // Parametros de iluminacion global (eyePos, ambientVal).
    // Se sincronizan con _buffers cada frame.
    GlobalLighting _lighting;
};
