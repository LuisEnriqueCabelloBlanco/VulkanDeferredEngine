#include "GPUBufferManager.h"

#include <cstring>
#include <stdexcept>

#include "VulkanDevice.h"
#include "Buffer.h"
#include "ResourceLimits.h"

static constexpr int FRAMES = ResourceLimits::MAX_FRAMES_IN_FLIGHT;
static constexpr int MAX_LIGHTS = ResourceLimits::MAX_LIGHTS;
static constexpr int MAX_CULL_OBJECTS = ResourceLimits::MAX_CULL_OBJECTS;

// ===========================================================================
// Ciclo de vida
// ===========================================================================

void GPUBufferManager::init(VulkanDevice& device)
{
    _device = &device;
    createCameraBuffers();
    createLightingBuffers();
    createCullingBuffers();
}

void GPUBufferManager::destroy()
{
    if (_device == nullptr) return;

    for (Buffer* b : _cameraVPBuffers) delete b;
    _cameraVPBuffers.clear();
    _cameraVPMapped.clear();

    delete _lightingUBO;        _lightingUBO = nullptr;
    delete _mainLightVPBuffer;  _mainLightVPBuffer = nullptr;
    delete _lightStorage;       _lightStorage = nullptr;
    delete _lightIndexStorage;  _lightIndexStorage = nullptr;
    delete _aabbStorage;        _aabbStorage = nullptr;
    delete _cameraCullIndex;    _cameraCullIndex = nullptr;
    delete _lightCullIndex;     _lightCullIndex = nullptr;

    _device = nullptr;
}

// ===========================================================================
// Creación interna
// ===========================================================================

void GPUBufferManager::createCameraBuffers()
{
    _cameraVPBuffers.resize(FRAMES);
    _cameraVPMapped.resize(FRAMES);

    const VkDeviceSize size = sizeof(ViewProjectionData);
    for (int i = 0; i < FRAMES; ++i) {
        _cameraVPBuffers[i] = _device->createBuffer(
            size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        _device->mapMemory(_cameraVPBuffers[i]->getMemory(), 0, size, &_cameraVPMapped[i]);
    }

    // Main light VP (shadow pass)
    _mainLightVPBuffer = _device->createBuffer(
        size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    _device->mapMemory(_mainLightVPBuffer->getMemory(), 0, size,
        reinterpret_cast<void**>(&_mainLightVPMapped));
}

void GPUBufferManager::createLightingBuffers()
{
    // GlobalLighting UBO
    const VkDeviceSize uboSize = sizeof(GlobalLighting);
    _lightingUBO = _device->createBuffer(
        uboSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    _device->mapMemory(_lightingUBO->getMemory(), 0, uboSize, &_lightingMapped);

    // LightObject storage (CPU-visible, escrito cada frame)
    // Layout: int count | 12 bytes padding | LightObject[MAX_LIGHTS]
    const VkDeviceSize storageSize =
        sizeof(int) + sizeof(LightObject) * MAX_LIGHTS + sizeof(uint8_t) * 16;
    _lightStorage = _device->createBuffer(
        storageSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    _device->mapMemory(_lightStorage->getMemory(), 0, storageSize, &_lightStorageMapped);

    // lightIndexStorage: escrito por el compute shader de light culling, solo GPU
    const VkDeviceSize indexSize = sizeof(int) + sizeof(int) * MAX_LIGHTS;
    _lightIndexStorage = _device->createBuffer(
        indexSize,
        static_cast<VkBufferUsageFlagBits>(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void GPUBufferManager::createCullingBuffers()
{
    // AABB storage (infraestructura preparada, uso comentado)
    const VkDeviceSize aabbSize =
        sizeof(int) + sizeof(AABBModel) * MAX_CULL_OBJECTS + sizeof(uint8_t) * 16;
    _aabbStorage = _device->createBuffer(
        aabbSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    _device->mapMemory(_aabbStorage->getMemory(), 0, aabbSize, &_aabbMapped);

    // Índices de culling por cámara y por luz
    const VkDeviceSize cullIndexSize =
        sizeof(int) + sizeof(int) * MAX_CULL_OBJECTS;

    _cameraCullIndex = _device->createBuffer(
        cullIndexSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    _device->mapMemory(_cameraCullIndex->getMemory(), 0, cullIndexSize, &_cameraCullMapped);

    _lightCullIndex = _device->createBuffer(
        cullIndexSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    _device->mapMemory(_lightCullIndex->getMemory(), 0, cullIndexSize, &_lightCullMapped);
}

// ===========================================================================
// Escrituras por frame
// ===========================================================================

void GPUBufferManager::writeCameraVP(uint32_t frameIndex, const ViewProjectionData& vp)
{
    // Invertimos Y para Vulkan (el origen del NDC está arriba)
    ViewProjectionData gpu = vp;
    gpu.proj[1][1] *= -1.f;
    memcpy(_cameraVPMapped[frameIndex], &gpu, sizeof(gpu));
}

void GPUBufferManager::writeLighting(const GlobalLighting& lighting)
{
    memcpy(_lightingMapped, &lighting, sizeof(lighting));
}

void GPUBufferManager::writeMainLightVP(const ViewProjectionData& vp)
{
    memcpy(_mainLightVPMapped, &vp, sizeof(vp));
}

void GPUBufferManager::writeLights(const std::vector<LightObject>& lights)
{
    if (lights.size() >= static_cast<size_t>(MAX_LIGHTS)) {
        throw std::runtime_error("GPUBufferManager::writeLights exceeded MAX_LIGHTS");
    }
    const int count = static_cast<int>(lights.size());
    memcpy(_lightStorageMapped, &count, sizeof(int));

    // +16 por alineamiento GPU (igual que antes)
    void* arrayStart = static_cast<uint8_t*>(_lightStorageMapped) + 16;
    memcpy(arrayStart, lights.data(), sizeof(LightObject) * lights.size());
}