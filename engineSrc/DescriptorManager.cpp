#include "DescriptorManager.h"

#include <array>
#include <stdexcept>

#include "VulkanDevice.h"
#include "RenderPipelines.h"
#include "GBuffer.h"
#include "ShadowPass.h"
#include "Texture.h"
#include "Buffer.h"

static constexpr int FRAMES   = ResourceLimits::MAX_FRAMES_IN_FLIGHT;
static constexpr int TEXTURES = ResourceLimits::MAX_TEXTURES;
static constexpr int LIGHTS   = ResourceLimits::MAX_LIGHTS;

// ===========================================================================
// Ciclo de vida
// ===========================================================================

void DescriptorManager::init( VulkanDevice& device, const RenderPipelines& pipelines )
{
    _device    = &device;
    _pipelines = &pipelines;

    createPool();
    createSets();
}

void DescriptorManager::destroy()
{
    if ( _pool != VK_NULL_HANDLE ) {
        _device->destroyDescriptorPool( _pool );
        _pool = VK_NULL_HANDLE;
    }
}

// ===========================================================================
// Creación del pool
// ===========================================================================

void DescriptorManager::createPool()
{
    // Conteo exacto de descriptores por tipo que se van a alocar en createSets():
    //
    //  UNIFORM_BUFFER
    //    _cameraDescriptorSets        : FRAMES × 1  (binding 0, VP)
    //    _globalLightingDescriptorSets: FRAMES × 1  (binding 3, GlobalLighting)
    //    _mainLightDescriptorSet      : 1     × 1  (binding 0, shadow VP)
    //    _computeDescriptorSets       : FRAMES × 1  (binding 2, GlobalLighting)
    //    Total: FRAMES*3 + 1
    //
    //  COMBINED_IMAGE_SAMPLER
    //    _textureArrayDescriptorSet   : 1 × MAX_TEXTURES  (binding 0, array)
    //    _globalLightingDescriptorSets: FRAMES × 1        (binding 6, shadow map)
    //    Total: MAX_TEXTURES + FRAMES
    //
    //  INPUT_ATTACHMENT
    //    _inputAttachmentsDescriptorSets: FRAMES × 3  (bindings 0-2, GBuffer)
    //    Total: FRAMES * 3
    //
    //  STORAGE_BUFFER
    //    _lightsDataBufferDescriptorSet: 1 × 2  (bindings 0-1, lightIndex + lightStorage)
    //    _cameraCullDescriptorSet      : 1 × 2  (bindings 0-1, cameraCullIndex + AABB)
    //    _lightCullDescriptorSet       : 1 × 2  (bindings 0-1, lightCullIndex + AABB)
    //    Total: 6

    std::array<VkDescriptorPoolSize, 4> sizes{};

    sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[0].descriptorCount = static_cast<uint32_t>( FRAMES * 3 + 1 );

    sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[1].descriptorCount = static_cast<uint32_t>( TEXTURES + FRAMES );

    sizes[2].type            = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    sizes[2].descriptorCount = static_cast<uint32_t>( FRAMES * 3 );

    sizes[3].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sizes[3].descriptorCount = 6u;

    // Número exacto de sets que se van a alocar en createSets():
    //   _cameraDescriptorSets           FRAMES
    //   _textureArrayDescriptorSet      1
    //   _inputAttachmentsDescriptorSets FRAMES
    //   _globalLightingDescriptorSets   FRAMES
    //   _lightsDataBufferDescriptorSet  1
    //   _mainLightDescriptorSet         1
    //   _computeDescriptorSets          FRAMES
    //   _cameraCullDescriptorSet        1
    //   _lightCullDescriptorSet         1
    //   Total: FRAMES * 4 + 5

    uint32_t maxSets = static_cast<uint32_t>( FRAMES * 4 + 5 );

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.poolSizeCount = static_cast<uint32_t>( sizes.size() );
    info.pPoolSizes    = sizes.data();
    info.maxSets       = maxSets;

    _pool = _device->createDescriptorPool( info );
}

// ===========================================================================
// Alocación de sets
// ===========================================================================

void DescriptorManager::createSets()
{
    // --- Geometry pass: VP UBO (FRAMES sets) --------------------------------
    {
        std::vector<VkDescriptorSetLayout> layouts( FRAMES, _pipelines->getViewProjectionLayout() );
        _cameraDescriptorSets = _device->createDescriptorSets( layouts, _pool );
    }

    // --- Geometry pass: texture array (1 set, variable descriptor count) ----
    {
        std::vector<VkDescriptorSetLayout> layouts( 1, _pipelines->getTextureArrayLayout() );
        uint32_t count = static_cast<uint32_t>( TEXTURES );

        VkDescriptorSetVariableDescriptorCountAllocateInfo varCount{};
        varCount.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
        varCount.descriptorSetCount = 1;
        varCount.pDescriptorCounts  = &count;

        _textureArrayDescriptorSet = _device->createDescriptorSets( layouts, _pool, &varCount )[0];
    }

    // --- Lighting pass: input attachments (FRAMES sets) ---------------------
    {
        std::vector<VkDescriptorSetLayout> layouts( FRAMES, _pipelines->getInputAttachmentsLayout() );
        _inputAttachmentsDescriptorSets = _device->createDescriptorSets( layouts, _pool );
    }

    // --- Lighting pass: GlobalLighting + shadow map (FRAMES sets) -----------
    {
        std::vector<VkDescriptorSetLayout> layouts( FRAMES, _pipelines->getDeferredDescriptorLayout() );
        _globalLightingDescriptorSets = _device->createDescriptorSets( layouts, _pool );
    }

    // --- Lighting pass: lights data storage (1 set único) ------------------
    {
        std::vector<VkDescriptorSetLayout> layouts( 1, _pipelines->getIndexedObjectsBufferLayout() );
        _lightsDataBufferDescriptorSet = _device->createDescriptorSets( layouts, _pool )[0];
    }

    // --- Shadow pass: main light VP (1 set único) ---------------------------
    {
        std::vector<VkDescriptorSetLayout> layouts( 1, _pipelines->getViewProjectionLayout() );
        _mainLightDescriptorSet = _device->createDescriptorSets( layouts, _pool )[0];
    }

    // --- Compute: GlobalLighting UBO (FRAMES sets) -------------------------
    {
        std::vector<VkDescriptorSetLayout> layouts( FRAMES, _pipelines->getComputeDescriptorLayout() );
        _computeDescriptorSets = _device->createDescriptorSets( layouts, _pool );
    }

    // --- Object culling: camera (1 set único) --------------------------------
    {
        std::vector<VkDescriptorSetLayout> layouts( 1, _pipelines->getIndexedObjectsBufferLayout() );
        _cameraCullDescriptorSet = _device->createDescriptorSets( layouts, _pool )[0];
    }

    // --- Object culling: light (1 set único) ---------------------------------
    {
        std::vector<VkDescriptorSetLayout> layouts( 1, _pipelines->getIndexedObjectsBufferLayout() );
        _lightCullDescriptorSet = _device->createDescriptorSets( layouts, _pool )[0];
    }
}

// ===========================================================================
// Helpers — factories de VkWriteDescriptorSet
// ===========================================================================

VkWriteDescriptorSet DescriptorManager::bufferWrite( VkDescriptorSet               dst,
                                                     uint32_t                      binding,
                                                     VkDescriptorType              type,
                                                     const VkDescriptorBufferInfo* info )
{
    VkWriteDescriptorSet w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = dst;
    w.dstBinding      = binding;
    w.dstArrayElement = 0;
    w.descriptorCount = 1;
    w.descriptorType  = type;
    w.pBufferInfo     = info;
    return w;
}

VkWriteDescriptorSet DescriptorManager::imageWrite( VkDescriptorSet              dst,
                                                    uint32_t                     binding,
                                                    VkDescriptorType             type,
                                                    const VkDescriptorImageInfo* info )
{
    VkWriteDescriptorSet w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = dst;
    w.dstBinding      = binding;
    w.dstArrayElement = 0;
    w.descriptorCount = 1;
    w.descriptorType  = type;
    w.pImageInfo      = info;
    return w;
}

// ===========================================================================
// Updates
// ===========================================================================

// ---------------------------------------------------------------------------
// updateGeometry: VP UBO (1 por frame) + texture array
// ---------------------------------------------------------------------------
void DescriptorManager::updateGeometry(
    const std::vector<Buffer*>&                              vpBuffers,
    const std::vector<ResourceManager::TextureBindingEntry>& textures )
{
    if ( textures.size() > static_cast<size_t>( TEXTURES ) ) {
        throw std::runtime_error( "DescriptorManager::updateGeometry: MAX_TEXTURES exceeded" );
    }

    // Construimos los VkDescriptorImageInfo antes del loop porque los writes
    // de textura apuntan al array de texturas (set único), no al set de frame.
    std::vector<VkDescriptorImageInfo> imgInfos;
    imgInfos.reserve( textures.size() );
    for ( const auto& entry : textures ) {
        imgInfos.push_back( entry.texture->getTextureDescriptor() );
    }

    for ( int i = 0; i < FRAMES; ++i ) {
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve( 1 + textures.size() );

        // binding 0: VP UBO → _cameraDescriptorSets[i]
        VkDescriptorBufferInfo vpInfo{};
        vpInfo.buffer = vpBuffers[i]->getBuffer();
        vpInfo.offset = 0;
        vpInfo.range  = sizeof( ViewProjectionData );
        writes.push_back( bufferWrite( _cameraDescriptorSets[i], 0,
                                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &vpInfo ) );

        // binding 0 (array): sampler por cada textura → _textureArrayDescriptorSet
        // Solo escribimos en la primera iteración: el set es único, no por frame.
        if ( i == 0 ) {
            for ( size_t t = 0; t < textures.size(); ++t ) {
                VkWriteDescriptorSet w{};
                w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w.dstSet          = _textureArrayDescriptorSet;
                w.dstBinding      = 0;
                w.dstArrayElement = textures[t].index;
                w.descriptorCount = 1;
                w.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                w.pImageInfo      = &imgInfos[t];
                writes.push_back( w );
            }
        }

        _device->updateDescriptorSet( writes );
    }
}

// ---------------------------------------------------------------------------
// updateLighting: input attachments + GlobalLighting UBO + shadow map +
//                 main light VP buffer
// ---------------------------------------------------------------------------
void DescriptorManager::updateLighting( const GBuffer&    gbuffer,
                                        const ShadowPass& shadowPass,
                                        Buffer*           lightUBO )
{
    const Texture* shadowMap = shadowPass.getShadowMap();

    for ( int i = 0; i < FRAMES; ++i ) {
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve( 6 );

        // --- Input attachments: binding 0 (albedo/color) --------------------
        VkDescriptorImageInfo colorInfo{};
        colorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorInfo.imageView   = gbuffer.getColorTexture()->textureImageView;
        colorInfo.sampler     = VK_NULL_HANDLE;
        writes.push_back( imageWrite( _inputAttachmentsDescriptorSets[i], 0,
                                      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &colorInfo ) );

        // --- Input attachments: binding 1 (normal) --------------------------
        VkDescriptorImageInfo normalInfo{};
        normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalInfo.imageView   = gbuffer.getNormalTexture()->textureImageView;
        normalInfo.sampler     = VK_NULL_HANDLE;
        writes.push_back( imageWrite( _inputAttachmentsDescriptorSets[i], 1,
                                      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &normalInfo ) );

        // --- Input attachments: binding 2 (position) ------------------------
        VkDescriptorImageInfo posInfo{};
        posInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        posInfo.imageView   = gbuffer.getPosTexture()->textureImageView;
        posInfo.sampler     = VK_NULL_HANDLE;
        writes.push_back( imageWrite( _inputAttachmentsDescriptorSets[i], 2,
                                      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &posInfo ) );

        // --- GlobalLighting UBO: binding 3 → _globalLightingDescriptorSets[i]
        VkDescriptorBufferInfo lightUBOInfo{};
        lightUBOInfo.buffer = lightUBO->getBuffer();
        lightUBOInfo.offset = 0;
        lightUBOInfo.range  = sizeof( GlobalLighting );
        writes.push_back( bufferWrite( _globalLightingDescriptorSets[i], 3,
                                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &lightUBOInfo ) );

        // --- Shadow map sampler: binding 6 → _globalLightingDescriptorSets[i]
        VkDescriptorImageInfo shadowInfo = shadowMap->getTextureDescriptor(
                                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL );
        writes.push_back( imageWrite( _globalLightingDescriptorSets[i], 6,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &shadowInfo ) );

        _device->updateDescriptorSet( writes );
    }
}

// ---------------------------------------------------------------------------
// updateCompute: GlobalLighting UBO → _computeDescriptorSets (binding 2)
// ---------------------------------------------------------------------------
void DescriptorManager::updateCompute( Buffer* lightingUBO )
{
    for ( int i = 0; i < FRAMES; ++i ) {
        VkDescriptorBufferInfo info{};
        info.buffer = lightingUBO->getBuffer();
        info.offset = 0;
        info.range  = sizeof( GlobalLighting );

        std::vector<VkWriteDescriptorSet> writes = {
            bufferWrite( _computeDescriptorSets[i], 2,
                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &info )
        };
        _device->updateDescriptorSet( writes );
    }
}

// ---------------------------------------------------------------------------
// updateShadow: main light VP → _mainLightDescriptorSet (binding 0)
// ---------------------------------------------------------------------------
void DescriptorManager::updateShadow( Buffer* mainLightVP )
{
    VkDescriptorBufferInfo info{};
    info.buffer = mainLightVP->getBuffer();
    info.offset = 0;
    info.range  = sizeof( ViewProjectionData );

    std::vector<VkWriteDescriptorSet> writes = {
        bufferWrite( _mainLightDescriptorSet, 0,
                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &info )
    };
    _device->updateDescriptorSet( writes );
}

// ---------------------------------------------------------------------------
// updateCulling: AABB + lightStorage → _lightsDataBufferDescriptorSet
//               cameraCullIndex + AABB → _cameraCullDescriptorSet
//               lightCullIndex  + AABB → _lightCullDescriptorSet
// ---------------------------------------------------------------------------
void DescriptorManager::updateCulling( Buffer* aabbStorage,
                                       Buffer* lightIndexStorage,
                                       Buffer* lightStorage,
                                       Buffer* cameraCullIndex,
                                       Buffer* lightCullIndex )
{
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve( 6 );

    // _lightsDataBufferDescriptorSet
    //   binding 0: lightIndex storage (índices de luces culled por el compute)
    VkDescriptorBufferInfo lightIdxInfo{};
    lightIdxInfo.buffer = lightIndexStorage->getBuffer();
    lightIdxInfo.offset = 0;
    lightIdxInfo.range  = sizeof( int ) + sizeof( int ) * LIGHTS;
    writes.push_back( bufferWrite( _lightsDataBufferDescriptorSet, 0,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &lightIdxInfo ) );

    //   binding 1: light data storage (posición, tipo, color de cada luz)
    VkDescriptorBufferInfo lightDataInfo{};
    lightDataInfo.buffer = lightStorage->getBuffer();
    lightDataInfo.offset = 0;
    lightDataInfo.range  = sizeof( int ) + sizeof( LightObject ) * LIGHTS + 16;
    writes.push_back( bufferWrite( _lightsDataBufferDescriptorSet, 1,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &lightDataInfo ) );

    // _cameraCullDescriptorSet
    //   binding 0: cameraCullIndex (output del compute de culling de objetos)
    VkDescriptorBufferInfo camIdxInfo{};
    camIdxInfo.buffer = cameraCullIndex->getBuffer();
    camIdxInfo.offset = 0;
    camIdxInfo.range  = VK_WHOLE_SIZE;
    writes.push_back( bufferWrite( _cameraCullDescriptorSet, 0,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &camIdxInfo ) );

    //   binding 1: AABB storage (input del compute de culling)
    VkDescriptorBufferInfo aabbForCamInfo{};
    aabbForCamInfo.buffer = aabbStorage->getBuffer();
    aabbForCamInfo.offset = 0;
    aabbForCamInfo.range  = VK_WHOLE_SIZE;
    writes.push_back( bufferWrite( _cameraCullDescriptorSet, 1,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &aabbForCamInfo ) );

    // _lightCullDescriptorSet
    //   binding 0: lightCullIndex
    VkDescriptorBufferInfo lightCullIdxInfo{};
    lightCullIdxInfo.buffer = lightCullIndex->getBuffer();
    lightCullIdxInfo.offset = 0;
    lightCullIdxInfo.range  = VK_WHOLE_SIZE;
    writes.push_back( bufferWrite( _lightCullDescriptorSet, 0,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &lightCullIdxInfo ) );

    //   binding 1: AABB storage
    VkDescriptorBufferInfo aabbForLightInfo{};
    aabbForLightInfo.buffer = aabbStorage->getBuffer();
    aabbForLightInfo.offset = 0;
    aabbForLightInfo.range  = VK_WHOLE_SIZE;
    writes.push_back( bufferWrite( _lightCullDescriptorSet, 1,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &aabbForLightInfo ) );

    _device->updateDescriptorSet( writes );
}

// ===========================================================================
// Getters
// ===========================================================================

VkDescriptorSet DescriptorManager::getCameraSet( uint32_t frameIndex ) const
{
    return _cameraDescriptorSets[frameIndex];
}

VkDescriptorSet DescriptorManager::getTextureArraySet() const
{
    return _textureArrayDescriptorSet;
}

VkDescriptorSet DescriptorManager::getInputAttachmentsSet( uint32_t frameIndex ) const
{
    return _inputAttachmentsDescriptorSets[frameIndex];
}

VkDescriptorSet DescriptorManager::getGlobalLightingSet( uint32_t frameIndex ) const
{
    return _globalLightingDescriptorSets[frameIndex];
}

VkDescriptorSet DescriptorManager::getMainLightSet() const
{
    return _mainLightDescriptorSet;
}

VkDescriptorSet DescriptorManager::getLightsDataBufferSet() const
{
    return _lightsDataBufferDescriptorSet;
}

VkDescriptorSet DescriptorManager::getComputeSet( uint32_t frameIndex ) const
{
    return _computeDescriptorSets[frameIndex];
}

VkDescriptorSet DescriptorManager::getCameraCullSet() const
{
    return _cameraCullDescriptorSet;
}

VkDescriptorSet DescriptorManager::getLightCullSet() const
{
    return _lightCullDescriptorSet;
}
