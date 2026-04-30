#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "BufferObjectsData.h"   // ViewProjectionData, GlobalLighting, LightObject
#include "ResourceLimits.h"      // MAX_FRAMES_IN_FLIGHT, MAX_LIGHTS

class VulkanDevice;
class Buffer;

// ---------------------------------------------------------------------------
// GPUBufferManager
// ---------------------------------------------------------------------------
/*
  Propietario del ciclo de vida de todos los buffers GPU del motor.

  RESPONSABILIDAD
  ---------------
  Crear, destruir y escribir (CPU->GPU) los buffers que RenderEngine necesita
  cada frame. No conoce descriptors, render passes ni la escena.

  CICLO DE VIDA
  -------------
    init()    — crea todos los buffers y persiste sus mappings.
    destroy() — libera todos los buffers.

  ESCRITURAS POR FRAME (llamar desde drawFrame())
  -----------------------------------------------
    writeCameraVP(frameIndex, vp)   — VP de la cámara principal -> uniformBuffers[frame]
    writeLighting(lighting)         — GlobalLighting UBO (eyePos, ambient)
    writeMainLightVP(vp)            — VP ortográfica de la main light -> shadow pass
    writeLights(lights)             — storage buffer de LightObjects

  GETTERS (usados por RenderEngine para pasar a DescriptorManager)
  ---------------------------------------------------------------
    getCameraBuffers()     - std::vector<Buffer*>& — uno por frame, para updateGeometry()
    getLightingUBO()       - Buffer*               — para updateLighting() y updateCompute()
    getMainLightVPBuffer() - Buffer*               — para updateShadow()
    getLightStorage()      - Buffer*               — para updateCulling()
    getLightIndexStorage() - Buffer*               — para updateCulling() y el pipeline barrier
    getAABBStorage()       - Buffer*               — para updateCulling()
    getCameraCullIndex()   - Buffer*               — para updateCulling()
    getLightCullIndex()    - Buffer*               — para updateCulling()
*/
class GPUBufferManager {
public:
    GPUBufferManager() = default;
    ~GPUBufferManager() = default;

    GPUBufferManager(const GPUBufferManager&) = delete;
    GPUBufferManager& operator=(const GPUBufferManager&) = delete;

    // -----------------------------------------------------------------------
    // Ciclo de vida
    // -----------------------------------------------------------------------

    void init(VulkanDevice& device);
    void destroy();

    // -----------------------------------------------------------------------
    // Escrituras por frame — CPU -> GPU
    // -----------------------------------------------------------------------

    // VP de la cámara principal al UBO del frame actual.
    // Invierte Y de la proyección para Vulkan (NDC clip space).
    void writeCameraVP(uint32_t frameIndex, const ViewProjectionData& vp);

    // GlobalLighting UBO: posición del ojo y valor de ambient.
    void writeLighting(const GlobalLighting& lighting);

    // VP ortográfica de la main light para el shadow pass.
    void writeMainLightVP(const ViewProjectionData& vp);

    // Vuelca la lista de luces activas al storage buffer.
    // Lanza std::runtime_error si lights.size() >= MAX_LIGHTS.
    void writeLights(const std::vector<LightObject>& lights);

    // -----------------------------------------------------------------------
    // Getters de Buffer* — para pasar a DescriptorManager
    // -----------------------------------------------------------------------

    const std::vector<Buffer*>& getCameraBuffers()     const { return _cameraVPBuffers; }
    Buffer* getLightingUBO()       const { return _lightingUBO; }
    Buffer* getMainLightVPBuffer() const { return _mainLightVPBuffer; }
    Buffer* getLightStorage()      const { return _lightStorage; }
    Buffer* getLightIndexStorage() const { return _lightIndexStorage; }
    Buffer* getAABBStorage()       const { return _aabbStorage; }
    Buffer* getCameraCullIndex()   const { return _cameraCullIndex; }
    Buffer* getLightCullIndex()    const { return _lightCullIndex; }

    // Acceso directo al VP mapeado de la main light.
    // Usado en drawFrame() para construir el ViewProjectionData antes de writeLights().
    const ViewProjectionData* getMainLightVPMapped() const { return _mainLightVPMapped; }

private:
    void createCameraBuffers();
    void createLightingBuffers();
    void createCullingBuffers();

    VulkanDevice* _device = nullptr;

    // --- Buffers de frame -----------------------------------------
    std::vector<Buffer*> _cameraVPBuffers;          // [MAX_FRAMES_IN_FLIGHT] VP UBO cámara
    std::vector<void*>   _cameraVPMapped;           // [MAX_FRAMES_IN_FLIGHT]

    Buffer* _lightingUBO = nullptr; // GlobalLighting UBO
    void* _lightingMapped = nullptr;

    Buffer* _mainLightVPBuffer = nullptr;
    ViewProjectionData* _mainLightVPMapped = nullptr;

    Buffer* _lightStorage = nullptr; // LightObject storage
    void* _lightStorageMapped = nullptr;

    // --- Buffer compute GPU ---------------------------------------
    Buffer* _lightIndexStorage = nullptr;           // escrito por compute shader

    // --- Object culling GPU ---------------------------------------
    Buffer* _aabbStorage = nullptr;
    void* _aabbMapped = nullptr;
    Buffer* _cameraCullIndex = nullptr;
    void* _cameraCullMapped = nullptr;
    Buffer* _lightCullIndex = nullptr;
    void* _lightCullMapped = nullptr;
};