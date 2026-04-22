#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "BufferObjectsData.h"
#include "Mesh.h"
#include "ResourceLimits.h"
#include "Texture.h"
#include "VulkanDevice.h"

enum class ResourceErrorCode {
    InvalidName,
    DuplicateName,
    LimitExceeded,
    InvalidHandle,
    StaleHandle,
    DependencyInUse,
    LoadFailed,
};

class ResourceException : public std::runtime_error {
public:
    ResourceException( ResourceErrorCode code, const std::string& message )
        : std::runtime_error( message ), _code( code ) {
    }

    ResourceErrorCode code() const {
        return _code;
    }

private:
    ResourceErrorCode _code;
};

/*
ResourceManager es el punto de propiedad y control de los recursos de alto nivel del motor.

Su responsabilidad es mantener tres registros independientes: meshes, textures y materiales.
Cada registro usa una tabla de slots con reutilizacion por free-list, un mapa de nombre a slot
para garantizar unicidad explicita, y handles con generation para detectar usos stale.

Capacidades principales:
- Crear recursos por nombre y rechazar duplicados.
- Liberar recursos en runtime.
- Resolver recursos por nombre o por handle.
- Proteger dependencias basicas entre recursos. Ej: entre materiales y texturas.
- Exponer la lista de texturas vivas para reconstruir los descriptor sets del renderer.

El objetivo es mantener el contrato simple: el nombre asegura unicidad,
el handle identifica el acceso tecnico seguro, y el manager es la unica capa que decide
si un recurso sigue siendo valido.
*/
class ResourceManager {
public:
    struct TextureBindingEntry {
        uint32_t index;
        const Texture* texture;
    };

    explicit ResourceManager( VulkanDevice& device );

    void clear();

    // Mesh lifecycle
    MeshHandle createMesh( const std::string& name, const std::string& path );
    MeshHandle createMesh( const std::string& name, const std::vector<Vertex>& vertices );
    MeshHandle createMesh( const std::string& name, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices );
    void releaseMesh( MeshHandle handle );
    void releaseAllMeshes();

    // Texture lifecycle
    TextureHandle createTexture( const std::string& name, const std::string& path );
    void releaseTexture( TextureHandle handle );
    void releaseAllTextures();

    // Material lifecycle
    MaterialHandle createMaterial( const std::string& name, const MaterialDesc& material );
    void releaseMaterial( MaterialHandle handle );
    void releaseAllMaterials();

    // Handle lookup by name
    MeshHandle tryGetMeshHandle( const std::string& name ) const;
    TextureHandle tryGetTextureHandle( const std::string& name ) const;
    MaterialHandle tryGetMaterialHandle( const std::string& name ) const;

    // Resource lookup by handle
    const Mesh* tryGetMesh( MeshHandle handle ) const;
    const Texture* tryGetTexture( TextureHandle handle ) const;
    const MaterialDesc* tryGetMaterial( MaterialHandle handle ) const;

    std::vector<TextureBindingEntry> getLiveTextureEntries() const;

private:
    // Representacion interna de cada tipo de recurso: slot + generation + nombre + puntero/valor.
    struct MeshSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        std::unique_ptr<Mesh> resource;
    };

    struct TextureSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        std::unique_ptr<Texture> resource;
    };

    struct MaterialSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        MaterialDesc resource;
    };

    // Utilidades comunes: validacion, construccion de handles y reserva de slots.
    static void bumpGeneration( uint32_t& generation );
    static void validateResourceName( const std::string& name, const char* callSite );
    static std::string makeDuplicateNameMessage( const char* callSite, const std::string& name );

    static MeshHandle makeMeshHandle( uint32_t index, uint32_t generation );
    static TextureHandle makeTextureHandle( uint32_t index, uint32_t generation );
    static MaterialHandle makeMaterialHandle( uint32_t index, uint32_t generation );

    uint32_t allocateMeshSlot();
    uint32_t allocateTextureSlot();
    uint32_t allocateMaterialSlot();

    // Reset seguro de slots cuando una creacion falla o cuando el recurso se libera.
    void resetMeshSlot( uint32_t index );
    void resetTextureSlot( uint32_t index );
    void resetMaterialSlot( uint32_t index );

    // Resolucion de acceso por handle con validacion de generation.
    const MeshSlot* tryGetMeshSlot( MeshHandle handle ) const;
    const TextureSlot* tryGetTextureSlot( TextureHandle handle ) const;
    const MaterialSlot* tryGetMaterialSlot( MaterialHandle handle ) const;

    // Version mutable usada al liberar recursos desde la API publica.
    MeshSlot& requireMeshSlot( MeshHandle handle, const char* callSite );
    TextureSlot& requireTextureSlot( TextureHandle handle, const char* callSite );
    MaterialSlot& requireMaterialSlot( MaterialHandle handle, const char* callSite );

    // Validacion de contratos cruzados: materiales solo pueden apuntar a texturas vivas.
    void validateMaterialTextures( const MaterialDesc& material ) const;
    bool isTextureUsedByAnyMaterial( TextureHandle textureHandle ) const;

private:
    VulkanDevice& _device;

    std::vector<MeshSlot> _meshSlots;
    std::vector<uint32_t> _freeMeshSlots;
    std::unordered_map<std::string, uint32_t> _meshNameToIndex;

    std::vector<TextureSlot> _textureSlots;
    std::vector<uint32_t> _freeTextureSlots;
    std::unordered_map<std::string, uint32_t> _textureNameToIndex;

    std::vector<MaterialSlot> _materialSlots;
    std::vector<uint32_t> _freeMaterialSlots;
    std::unordered_map<std::string, uint32_t> _materialNameToIndex;
};
