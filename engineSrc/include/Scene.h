#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "BufferObjectsData.h"
#include "Transform.h"

// ---------------------------------------------------------------------------
// Dominio de errores
// ---------------------------------------------------------------------------

enum class SceneErrorCode {
    InvalidHandle,   // El handle no fue emitido por este Scene o nunca fue valido.
    StaleHandle,     // El handle era valido pero la entidad fue destruida posteriormente.
    LimitExceeded,   // Se ha superado el numero maximo de entidades permitidas.
};

class SceneException : public std::runtime_error {
public:
    SceneException( SceneErrorCode code, const std::string& message )
        : std::runtime_error( message ), _code( code ) {
    }

    SceneErrorCode code() const { return _code; }

private:
    SceneErrorCode _code;
};

// ---------------------------------------------------------------------------

static constexpr uint32_t MAX_ENTITIES = 16384;

class RenderEntityHandle;

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

/*
Scene es el contenedor de entidades de la escena y la unica interfaz publica
de la capa de escena para la aplicacion.

Internamente usa una slot table con free-list y contadores de generacion:
  - Acceso O(1) por indice.
  - Deteccion segura de handles stale despues de destroyEntity().
  - Reutilizacion de slots sin crecer indefinidamente el vector.

La aplicacion crea y destruye entidades a traves de RenderEntityHandle,
emitidos exclusivamente por Scene::createEntity().

EntitySlot es un detalle de implementacion privado: la aplicacion nunca
puede obtener una referencia a el directamente.

buildRenderQueue() esta reservado para RenderEngine (friend).
requireSlot / requireSlotConst estan reservados para RenderEntityHandle (friend).
*/
class Scene {
public:
    Scene()  = default;
    ~Scene() = default;

    Scene( const Scene& ) = delete;
    Scene& operator=( const Scene& ) = delete;
    Scene( Scene&& ) = delete;
    Scene& operator=( Scene&& ) = delete;

    // --- Ciclo de vida de entidades -----------------------------------------

    // Crea una entidad vacia y devuelve su handle.
    // Lanza SceneException(LimitExceeded) si se supera MAX_ENTITIES.
    RenderEntityHandle createEntity();

    // Crea una entidad con mesh, material y transform ya asignados.
    // Lanza SceneException(LimitExceeded) si se supera MAX_ENTITIES.
    RenderEntityHandle createEntity( MeshHandle mesh,
                                     MaterialHandle material,
                                     const Transform& transform );

    // Destruye la entidad referenciada por el handle.
    // Tras esta llamada, handle.isValid() devuelve false.
    // Lanza SceneException(InvalidHandle) si el handle no fue inicializado
    // o la escena es inválida.
    // Lanza SceneException(StaleHandle) si la entidad correspondiente al
    // handle ya habia sido destruida.
    void destroyEntity( RenderEntityHandle& handle );

    // Destruye todas las entidades de forma logica y ordenada.
    // Los handles existentes pasan a estado stale (no invalid).
    void clear();

    // --- Consultas generales ------------------------------------------------

    // No lanza excepciones: devuelve false para handles invalidos o stale.
    bool        hasEntity( const RenderEntityHandle& handle ) const;
    std::size_t entityCount() const;

private:

    // --- Acceso interno a slots (usado por RenderEntityHandle) --------------

    friend class RenderEntityHandle;
    
    struct EntitySlot {
        uint32_t     generation  = 1;
        bool         occupied    = false;
        bool         active      = true;
        bool         visible     = true;
        RenderObject renderObject;
    };

    // Devuelve el slot mutable o lanza SceneException si el handle es invalido o stale.
    EntitySlot& requireSlot( uint32_t index, uint32_t generation, const char* callSite );

    // Version const para consultas.
    const EntitySlot& requireSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const;


    // --- Render queue (exclusivo para RenderEngine) -------------------------

    friend class RenderEngine;

    // Llamado por RenderEngine cada frame. Devuelve referencia a cache interna.
    const std::vector<RenderObject>& buildRenderQueue() const;

    
    // --- Implementacion interna ---------------------------------------------

    bool hasEntity( uint32_t index, uint32_t generation ) const;

    EntitySlot*       tryGetSlot( uint32_t index, uint32_t generation );
    const EntitySlot* tryGetSlot( uint32_t index, uint32_t generation ) const;

    bool     isSlotIndexInRange( uint32_t index ) const;
    uint32_t getSlotGeneration( uint32_t index )  const;
    uint32_t allocateSlot();

    static void bumpGeneration( uint32_t& generation );

private:
    std::vector<EntitySlot>           _slots;
    std::vector<uint32_t>             _freeSlots;
    mutable std::vector<RenderObject> _renderQueueCache;
};
