#pragma once

#include <cstdint>

#include <glm/vec4.hpp>

#include "ResourceHandles.h"

class ResourceManager;

struct MaterialCreateInfo {
    glm::vec4     baseColor = glm::vec4( 1.0f );
    float         metallic = 0.0f;
    float         roughness = 1.0f;
    TextureHandle baseColorTexture{};
    TextureHandle normalTexture{};
};

// ---------------------------------------------------------------------------
// MaterialHandle
// ---------------------------------------------------------------------------

/*
MaterialHandle es el token que la aplicacion recibe al crear un material.

Encapsula el par (index, generation) que identifica un MaterialSlot en
ResourceManager, mas un puntero al manager que lo emitio.

Toda mutacion y consulta de propiedades pasa por este handle, que valida:
- handle inicializado,
- handle emitido por el ResourceManager correcto,
- generation vigente (no stale),
- y, para texturas, que el handle de textura siga vivo.

La fuente de verdad del material es el slot del ResourceManager. El handle no
mantiene copia local de estado; actua como punto de acceso seguro.

Uso tipico:
    MaterialCreateInfo info;
    info.baseColor = glm::vec4( 1.0f );
    info.roughness = 0.5f;

    MaterialHandle mat = resources.createMaterial( "mat_floor", info );
    mat.setMetallic( 0.2f );
    mat.setBaseColorTexture( albedoTexture );
*/
class MaterialHandle {
public:
    MaterialHandle() = default;

    bool isValid() const;

    void setBaseColor( const glm::vec4& baseColor );
    void setMetallic( float metallic );
    void setRoughness( float roughness );
    void setBaseColorTexture( TextureHandle texture );
    void setNormalTexture( TextureHandle texture );

    glm::vec4 getBaseColor() const;
    float getMetallic() const;
    float getRoughness() const;
    TextureHandle getBaseColorTexture() const;
    TextureHandle getNormalTexture() const;

    void clearBaseColorTexture();
    void clearNormalTexture();

private:
    friend class ResourceManager;

    MaterialHandle( uint32_t index, uint32_t generation, ResourceManager* manager )
        : _index( index ), _generation( generation ), _manager( manager ) {
    }

private:
    uint32_t _index = INVALID_HANDLE_INDEX;
    uint32_t _generation = INVALID_HANDLE_GENERATION;
    ResourceManager* _manager = nullptr;
};
