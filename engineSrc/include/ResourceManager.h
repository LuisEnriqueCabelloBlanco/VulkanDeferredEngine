#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec4.hpp>

#include "BufferObjectsData.h"
#include "ResourceHandles.h"
#include "Mesh.h"
#include "ResourceLimits.h"
#include "Texture.h"
#include "VulkanDevice.h"

struct MaterialCreateInfo {
    glm::vec4     baseColor = glm::vec4( 1.0f );
    float         metallic = 0.0f;
    float         roughness = 1.0f;
    TextureHandle baseColorTexture{};
    TextureHandle normalTexture{};
};

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
    // Entrada que representa una textura viva lista para bind en descriptor arrays.
    // index: indice real del slot en la tabla de texturas del manager.
    // texture: recurso GPU asociado al slot.
    struct TextureBindingEntry {
        uint32_t index;
        const Texture* texture;
    };

    explicit ResourceManager( VulkanDevice& device );

    // Limpia TODO el estado del manager.
    // Orden interno: materiales -> texturas -> meshes para respetar dependencias.
    // Si clear() falla en una fase intermedia, puede dejar estado parcialmente limpiado.
    void clear();

    // --- Ciclo de vida de Mesh ---------------------------------------------
    // createMesh(...): crea un mesh unico por nombre y devuelve handle valido.
    // Lanza ResourceException si el nombre es invalido, duplicado, se supera
    // el limite o falla la carga/creacion interna.
    // releaseMesh(...): invalida el handle (bump generation del slot).
    // releaseAllMeshes(): libera todos los meshes ocupados.
    [[nodiscard]] MeshHandle createMesh( const std::string& name, 
                                         const std::string& path );

    [[nodiscard]] MeshHandle createMesh( const std::string& name, 
                                         const std::vector<Vertex>& vertices );

    [[nodiscard]] MeshHandle createMesh( const std::string& name, 
                                         const std::vector<uint32_t>& indices, 
                                         const std::vector<Vertex>& vertices );
                                         
    void releaseMesh( MeshHandle handle );
    void releaseAllMeshes();

    // --- Ciclo de vida de Texture ------------------------------------------
    // createTexture(...): crea textura unica por nombre.
    // Lanza ResourceException ante nombre invalido, duplicado, limite o fallo
    // de carga.
    // releaseTexture(...): rechaza liberar si existe dependencia de materiales.
    // releaseAllTextures(): idem a nivel global; exige no tener materiales vivos
    // que referencien texturas.
    [[nodiscard]] TextureHandle createTexture( const std::string& name, const std::string& path );
    void releaseTexture( TextureHandle handle );
    void releaseAllTextures();

    // --- Ciclo de vida de Material -----------------------------------------
    // createMaterial(...): valida texturas referenciadas y persiste datos GPU.
    // Lanza ResourceException ante nombre invalido, duplicado, limite,
    // dependencia invalida o fallo de persistencia interna.
    // releaseMaterial/releaseAllMaterials: liberan slots e invalidan handles.
    [[nodiscard]] MaterialHandle createMaterial( const std::string& name, const MaterialCreateInfo& material );
    void releaseMaterial( MaterialHandle handle );
    void releaseAllMaterials();

    // --- Lookup por nombre --------------------------------------------------
    // tryGet*Handle(...): devuelve handle valido si el recurso sigue vivo,
    // o handle nulo (default-constructed) si no existe/no esta ocupado.
    MeshHandle tryGetMeshHandle( const std::string& name ) const;
    TextureHandle tryGetTextureHandle( const std::string& name ) const;
    MaterialHandle tryGetMaterialHandle( const std::string& name ) const;

    // --- Enumeracion de recursos vivos --------------------------------------
    // Devuelven los nombres de todos los recursos actualmente ocupados.
    // Util para inspectores, herramientas de debug y serializacion.
    // El orden no esta garantizado.
    std::vector<std::string> getMeshNames()     const;
    std::vector<std::string> getTextureNames()  const;
    std::vector<std::string> getMaterialNames() const;

private:

    // --- Slots de recursos mallas (privados) --------------------------------
    struct MeshSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        std::unique_ptr<Mesh> resource;
    };

    // --- Slots de recursos texturas (privados) ------------------------------
    struct TextureSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        std::unique_ptr<Texture> resource;
    };

    // --- Slots de objetos materiales (privados) -----------------------------
    struct MaterialSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        MaterialData data{};
    };

    // --- Acceso interno para RenderEngine ----------------------------------

    friend class RenderEngine;

    // Avisa al RenderEngine cuando cambian las texturas vivas para que
    // reconstruya el array de descriptors de forma transparente para la app.
    void setTextureBindingsChangedCallback( std::function<void()> callback );
    
    void notifyTextureBindingsChanged() const;

    // Devuelve el conjunto de texturas vivas con su indice real de slot.
    // Usado por RenderEngine para actualizar sus descriptor sets.
    std::vector<TextureBindingEntry> getLiveTextureEntries() const;


    // --- Acceso interno para RenderEngine y CullManager --------------------

    friend class CullManager;

    // tryGet* de recursos concretos: lookup no-throw por handle.
    // Si el handle es invalido/stale, devuelve nullptr.
    const Mesh* tryGetMesh(MeshHandle handle) const;
    const Texture* tryGetTexture(TextureHandle handle) const;
    const MaterialData* tryGetMaterial(MaterialHandle handle) const;


    // --- Implementacion interna ---------------------------------------------

    // Utilidades comunes: validacion, construccion de handles y reserva de slots.
    // bumpGeneration: invalida handles previos de un slot.
    // validateResourceName: contrato comun de nombre no vacio.
    static void bumpGeneration( uint32_t& generation );
    static void validateResourceName( const std::string& name, const char* callSite );
    static std::string makeDuplicateNameMessage( const char* callSite, const std::string& name );

    // Helpers para crear Handles
    MeshHandle makeMeshHandle( uint32_t index, uint32_t generation ) const;
    TextureHandle makeTextureHandle( uint32_t index, uint32_t generation ) const;
    MaterialHandle makeMaterialHandle( uint32_t index, uint32_t generation ) const;

    // Reserva de slots por pool + free-list.
    // Si no hay huecos libres, puede crecer hasta el limite ResourceLimits.
    uint32_t allocateMeshSlot();
    uint32_t allocateTextureSlot();
    uint32_t allocateMaterialSlot();

    // Reset y release de slots:
    // - reset*Slot: rollback de create* fallido (NO bump generation).
    // - release*SlotByIndex: teardown comun reutilizable. bumpGen controla
    //   si se invalida generacion (true en release normal, false en rollback).
    void resetMeshSlot( uint32_t index );
    void resetTextureSlot( uint32_t index );
    void resetMaterialSlot( uint32_t index );
    void releaseMeshSlotByIndex( uint32_t index, bool bumpGen );
    void releaseTextureSlotByIndex( uint32_t index, bool bumpGen );
    void releaseMaterialSlotByIndex( uint32_t index, bool bumpGen );

    // Resolucion interna por handle (no-throw):
    // devuelve nullptr si el handle es invalido, out-of-range, slot libre o stale.
    const MeshSlot* tryGetMeshSlot( MeshHandle handle ) const;
    const TextureSlot* tryGetTextureSlot( TextureHandle handle ) const;
    const MaterialSlot* tryGetMaterialSlot( MaterialHandle handle ) const;

    // Resolucion interna por handle (throw):
    // lanza InvalidHandle o StaleHandle con contexto callSite.
    MeshSlot& requireMeshSlot( MeshHandle handle, const char* callSite );
    TextureSlot& requireTextureSlot( TextureHandle handle, const char* callSite );
    MaterialSlot& requireMaterialSlot( MaterialHandle handle, const char* callSite );

    // Contratos cruzados de materiales/texturas:
    // validateMaterialTextureHandle: acepta handle nulo ("sin textura"),
    // rechaza handles fuera de rango o stale.
    // isTextureUsedByAnyMaterial: detecta si hay materiales ocupados que
    // referencian el indice de textura.
    void validateMaterialTextureHandle( TextureHandle textureHandle, const char* callSite ) const;
    bool isTextureUsedByAnyMaterial( TextureHandle textureHandle ) const;

    
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

    // Callback para notificar al renderer que debe reconstruir bindings.
    std::function<void()> _textureBindingsChangedCallback;
};
