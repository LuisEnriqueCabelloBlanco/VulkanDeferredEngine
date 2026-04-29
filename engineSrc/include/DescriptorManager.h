#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "ResourceLimits.h"
#include "ResourceManager.h"   // TextureBindingEntry
#include "BufferObjectsData.h" // ViewProjectionData, GlobalLighting

class VulkanDevice;
class RenderPipelines;
class GBuffer;
class ShadowPass;
class Buffer;

// ---------------------------------------------------------------------------
// DescriptorManager
// ---------------------------------------------------------------------------
/*
  Propietario del VkDescriptorPool y de todos los VkDescriptorSets del engine.

  CICLO DE VIDA
  -------------
    init()        — crea el pool y aloca todos los sets (vacíos).
    updateXxx()   — escribe los sets de cada grupo con los datos exactos
                    que RenderEngine le pasa. Pueden llamarse tantas veces
                    como sea necesario.
    destroy()     — destruye el pool (libera todos los sets implícitamente).

  UPDATES
  -------
    · updateCompute()  — Llamar una vez tras crear los buffers.
    · updateShadow()   — Llamar una vez tras crear el buffer.
    · updateCulling()  — Llamar una vez tras crear los buffers.
    · updateGeometry() — Llamar tras cargar/liberar texturas.
    · updateLighting() — Llamar tras cualquier resize (GBuffer recreado).

  LO QUE NO GESTIONA ESTA CLASE
  ------------------------------
  Los buffers GPU, los layouts de descriptor, el GBuffer y el ShadowPass son
  propiedad de RenderEngine / RenderPipelines. DescriptorManager los recibe
  como parámetros y nunca los destruye.
*/
class DescriptorManager {
public:
    DescriptorManager()  = default;
    ~DescriptorManager() = default;

    DescriptorManager( const DescriptorManager& ) = delete;
    DescriptorManager& operator=( const DescriptorManager& ) = delete;

    // -----------------------------------------------------------------------
    // Ciclo de vida
    // -----------------------------------------------------------------------

    void init( VulkanDevice& device, const RenderPipelines& pipelines );
    void destroy();

    // -----------------------------------------------------------------------
    // Updates
    // -----------------------------------------------------------------------

    // VP UBO (1 por frame) + array de samplers.
    // Llamar tras cargar o liberar texturas.
    void updateGeometry( const std::vector<Buffer*>&                              vpBuffers,
                         const std::vector<ResourceManager::TextureBindingEntry>& textures );

    // Input attachments del GBuffer + shadow map + GlobalLighting UBO.
    // Llamar tras cualquier resize (el GBuffer se ha recreado).
    void updateLighting( const GBuffer&    gbuffer,
                         const ShadowPass& shadowPass,
                         Buffer*           lightUBO );

    // GlobalLighting UBO → sets del compute de light culling.
    // Llamar una vez tras crear los buffers.
    void updateCompute( Buffer* lightingUBO );

    // VP buffer de la main light → set del shadow pass.
    // Llamar una vez tras crear el buffer.
    void updateShadow( Buffer* mainLightVP );

    // Buffers de culling (AABB, lightIndex, lightStorage, índices).
    // Llamar una vez tras crear los buffers.
    void updateCulling( Buffer* aabbStorage,
              Buffer* lightIndexStorage,
              Buffer* lightStorage,
              Buffer* cameraCullIndex,
              Buffer* lightCullIndex );

    // -----------------------------------------------------------------------
    // Getters — usados en RenderEngine::record*()
    // -----------------------------------------------------------------------

    VkDescriptorSet getCameraSet( uint32_t frameIndex )           const;
    VkDescriptorSet getTextureArraySet()                          const;
    VkDescriptorSet getInputAttachmentsSet( uint32_t frameIndex ) const;
    VkDescriptorSet getGlobalLightingSet( uint32_t frameIndex )   const;
    VkDescriptorSet getMainLightSet()                             const;
    VkDescriptorSet getLightsDataBufferSet()                      const;
    VkDescriptorSet getComputeSet( uint32_t frameIndex )          const;
    VkDescriptorSet getCameraCullSet()                            const;
    VkDescriptorSet getLightCullSet()                             const;

private:
    // -----------------------------------------------------------------------
    // Helpers internos
    // -----------------------------------------------------------------------
    void createPool();
    void createSets();

    static VkWriteDescriptorSet bufferWrite( VkDescriptorSet dst, uint32_t binding,
                                             VkDescriptorType type,
                                             const VkDescriptorBufferInfo* info );

    static VkWriteDescriptorSet imageWrite( VkDescriptorSet dst, uint32_t binding,
                                            VkDescriptorType type,
                                            const VkDescriptorImageInfo* info );

    // -----------------------------------------------------------------------
    // Dependencias (no poseídas)
    // -----------------------------------------------------------------------
    VulkanDevice*          _device    = nullptr;
    const RenderPipelines* _pipelines = nullptr;

    // -----------------------------------------------------------------------
    // Pool
    // -----------------------------------------------------------------------
    VkDescriptorPool _pool = VK_NULL_HANDLE;

    // -----------------------------------------------------------------------
    // Sets
    // -----------------------------------------------------------------------

    // Geometry pass (subpass 0)
    std::vector<VkDescriptorSet> _cameraDescriptorSets;           // [MAX_FRAMES_IN_FLIGHT]
    VkDescriptorSet              _textureArrayDescriptorSet = VK_NULL_HANDLE;

    // Lighting pass (subpass 1)
    std::vector<VkDescriptorSet> _inputAttachmentsDescriptorSets; // [MAX_FRAMES_IN_FLIGHT]
    std::vector<VkDescriptorSet> _globalLightingDescriptorSets;   // [MAX_FRAMES_IN_FLIGHT]
    VkDescriptorSet              _lightsDataBufferDescriptorSet = VK_NULL_HANDLE;

    // Shadow pass
    VkDescriptorSet _mainLightDescriptorSet = VK_NULL_HANDLE;

    // Compute — light culling
    std::vector<VkDescriptorSet> _computeDescriptorSets;          // [MAX_FRAMES_IN_FLIGHT]

    // Object culling (compute)
    VkDescriptorSet _cameraCullDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet _lightCullDescriptorSet  = VK_NULL_HANDLE;
};
