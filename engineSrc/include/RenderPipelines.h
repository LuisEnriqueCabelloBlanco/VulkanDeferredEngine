#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

// ---------------------------------------------------------------------------
// RenderPipelines
// ---------------------------------------------------------------------------

/*
RenderPipelines es el propietario de todos los objetos de configuracion
estatica de la GPU: descriptor set layouts, pipeline layouts y pipelines.

Los tres tienen el mismo ciclo de vida: se crean una vez en init() y se
destruyen en destroy(). Ninguno de ellos cambia en runtime.

DESCRIPTOR SET LAYOUTS
    Describen la estructura que los shaders esperan en cada binding. Son
    prerequisito para los pipeline layouts. RenderEngine los usa tambien
    para crear los descriptor sets contra el pool.
    Ver: createDescriptorSetLayouts()

PIPELINE LAYOUTS
    Agrupan descriptor set layouts y push constant ranges. Son prerequisito
    para las pipelines. RenderEngine los usa durante el recording.
    Ver: createXxxPipeline() — cada pipeline crea su propio layout internamente.

PIPELINES
    Estado completo de renderizado compilado.
    Cada pipeline conoce su propia configuracion.
    RenderPipelines no recibe parametros de configuracion desde fuera salvo
    los render passes, que son el unico dato verdaderamente externo.
    Ver: createXxxPipeline()

PARAMETROS DE INIT
    device          — necesario para crear y destruir todos los objetos Vk.
    mainRenderPass  — usado por la graphics pipeline (subpass 0) y la
                      deferred pipeline (subpass 1).
    shadowRenderPass — usado por la shadow pipeline.

LO QUE NO GESTIONA ESTA CLASE
    - Descriptor sets: son instancias runtime que apuntan a buffers concretos.
      Tienen su propio ciclo de vida y cambian en resize y cuando cambian las texturas.
    - Descriptor pool: memoria de la que se asignan los sets.
    - Render passes: se crean antes que las pipelines y se pasan como parametros.
*/
class RenderPipelines {
public:
    RenderPipelines() = default;
    ~RenderPipelines() = default;

    RenderPipelines( const RenderPipelines& ) = delete;
    RenderPipelines& operator=( const RenderPipelines& ) = delete;

    // Crea todos los descriptor set layouts, pipeline layouts y pipelines.
    // Debe llamarse despues de que mainRenderPass y shadowRenderPass esten creados.
    void init( VulkanDevice& device,
               VkRenderPass  mainRenderPass,
               VkRenderPass  shadowRenderPass );

    // Destruye todos los objetos en orden inverso al de creacion.
    void destroy();

    // --- Pipelines ----------------------------------------------------------
    // Usadas en RenderEngine::recordCommandBuffer() para vkCmdBindPipeline.

    VkPipeline getGraphicsPipeline()   const { return _graphicsPipeline;   }
    VkPipeline getDeferredPipeline()   const { return _deferredPipeline;   }
    VkPipeline getShadowPipeline()     const { return _shadowPipeline;     }
    VkPipeline getComputePipeline()    const { return _computePipeline;    }
    VkPipeline getObjectCullPipeline() const { return _objectCullPipeline; }

    // --- Pipeline layouts ---------------------------------------------------
    // Usados en vkCmdBindDescriptorSets y vkCmdPushConstants durante el recording.

    VkPipelineLayout getGraphicsLayout()   const { return _graphicsLayout;   }
    VkPipelineLayout getDeferredLayout()   const { return _deferredLayout;   }
    VkPipelineLayout getShadowLayout()     const { return _shadowLayout;     }
    VkPipelineLayout getComputeLayout()    const { return _computeLayout;    }
    VkPipelineLayout getObjectCullLayout() const { return _objectCullLayout; }

    // --- Descriptor set layouts ---------------------------------------------
    // Usados por RenderEngine para crear los descriptor sets contra el pool.
    // Tambien son prerequisito interno de los pipeline layouts.

    VkDescriptorSetLayout getViewProjectionLayout()       const { return _viewProjectionLayout;       }
    VkDescriptorSetLayout getTextureArrayLayout()         const { return _textureArrayLayout;         }
    VkDescriptorSetLayout getInputAttachmentsLayout()     const { return _inputAttachmentsLayout;     }
    VkDescriptorSetLayout getDeferredDescriptorLayout()   const { return _deferredDescriptorLayout;   }
    VkDescriptorSetLayout getComputeDescriptorLayout()    const { return _computeDescriptorLayout;    }
    VkDescriptorSetLayout getIndexedObjectsBufferLayout() const { return _indexedObjectsBufferLayout; }

private:
    // --- Creacion de descriptor set layouts ---------------------------------
    // Llamados al inicio de init(), antes que cualquier pipeline layout.

    void createDescriptorSetLayouts();

    void createViewProjectionLayout();       // binding 0: VP UBO → ALL_GRAPHICS | COMPUTE
    void createTextureArrayLayout();         // binding 0: array variable de samplers → FRAGMENT
    void createInputAttachmentsLayout();     // bindings 0-2: input attachments del GBuffer → FRAGMENT
    void createDeferredDescriptorLayout();   // binding 3: GlobalLighting UBO, binding 6: shadowMap sampler → FRAGMENT
    void createComputeDescriptorLayout();    // binding 2: GlobalLighting UBO → COMPUTE
    void createIndexedObjectsBufferLayout(); // binding 0: index buffer, binding 1: object/light buffer → COMPUTE | FRAGMENT

    // --- Creacion de pipelines ----------------------------------------------
    // Cada metodo crea su propio pipeline layout internamente antes de la pipeline.
    // Los shader paths son conocimiento fijo de cada pipeline.

    void createGraphicsPipeline();    // shaders: vertex + fragment         subpass 0 del main render pass
    void createDeferredPipeline();    // shaders: readAttachmentVertex + deferred   subpass 1 del main render pass
    void createShadowPipeline();      // shader:  shadowVertex               shadow render pass
    void createComputePipeline();     // shader:  lightCull                  compute (sin render pass)
    void createObjectCullPipeline();  // shader:  objectCull                 compute (sin render pass)

    // --- Destruccion --------------------------------------------------------

    void destroyPipelines();
    void destroyPipelineLayouts();
    void destroyDescriptorSetLayouts();

    // --- Dependencias externas (no poseidas) --------------------------------

    VulkanDevice* _device          = nullptr;
    VkRenderPass  _mainRenderPass  = VK_NULL_HANDLE;
    VkRenderPass  _shadowRenderPass = VK_NULL_HANDLE;

    // --- Descriptor set layouts ---------------------------------------------

    VkDescriptorSetLayout _viewProjectionLayout       = VK_NULL_HANDLE;
    VkDescriptorSetLayout _textureArrayLayout         = VK_NULL_HANDLE;
    VkDescriptorSetLayout _inputAttachmentsLayout     = VK_NULL_HANDLE;
    VkDescriptorSetLayout _deferredDescriptorLayout   = VK_NULL_HANDLE;
    VkDescriptorSetLayout _computeDescriptorLayout    = VK_NULL_HANDLE;
    VkDescriptorSetLayout _indexedObjectsBufferLayout = VK_NULL_HANDLE;

    // --- Pipeline layouts ---------------------------------------------------

    VkPipelineLayout _graphicsLayout   = VK_NULL_HANDLE;
    VkPipelineLayout _deferredLayout   = VK_NULL_HANDLE;
    VkPipelineLayout _shadowLayout     = VK_NULL_HANDLE;
    VkPipelineLayout _computeLayout    = VK_NULL_HANDLE;
    VkPipelineLayout _objectCullLayout = VK_NULL_HANDLE;

    // --- Pipelines ----------------------------------------------------------

    VkPipeline _graphicsPipeline   = VK_NULL_HANDLE;
    VkPipeline _deferredPipeline   = VK_NULL_HANDLE;
    VkPipeline _shadowPipeline     = VK_NULL_HANDLE;
    VkPipeline _computePipeline    = VK_NULL_HANDLE;
    VkPipeline _objectCullPipeline = VK_NULL_HANDLE;
};
