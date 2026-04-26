#pragma once

#include <cstdint>
#include <optional>
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
    LimitExceeded,   // Se ha superado el numero maximo de entidades/luces permitidas.
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
static constexpr uint32_t MAX_LIGHTS   = 4096;

class RenderEntityHandle;
class LightEntityHandle;

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

/*
Scene es el contenedor de entidades y luces de la escena, y la unica
interfaz publica de la capa de escena para la aplicacion.

Internamente usa slot tables independientes para entidades renderizables y
para luces, ambas con free-list y contadores de generacion.

La aplicacion crea y destruye entidades a traves de RenderEntityHandle y
luces a traves de LightEntityHandle, emitidos exclusivamente por Scene.

buildRenderQueue() y buildLightQueue() estan reservados para RenderEngine.
requireSlot / requireLightSlot estan reservados para los handles (friend).
*/
class Scene {
public:
    Scene()  = default;
    ~Scene() = default;

    Scene( const Scene& ) = delete;
    Scene& operator=( const Scene& ) = delete;
    Scene( Scene&& ) = delete;
    Scene& operator=( Scene&& ) = delete;


    // --- Ciclo de vida de entidades renderizables ---------------------------

    // Crea una entidad vacia y devuelve su handle.
    RenderEntityHandle createEntity();

    // Crea una entidad con mesh, material y transform ya asignados.
    RenderEntityHandle createEntity( MeshHandle mesh,
                                     MaterialHandle material,
                                     const Transform& transform );

    // Destruye la entidad referenciada por el handle e invalida su generacion.
    // Lanza SceneException(InvalidHandle) si el handle nunca fue valido.
    // Lanza SceneException(StaleHandle)   si la entidad ya fue destruida antes.
    void destroyEntity( RenderEntityHandle& handle );
    

    // --- Ciclo de vida de luces ---------------------------------------------

    // Crea una luz y devuelve su handle.
    // posOrDir: posicion en espacio mundo para Point/Spotlight,
    //           direccion normalizada para Directional.
    // range: solo relevante para Point y Spotlight; ignorado en Directional.
    LightEntityHandle createLight( LightType         type,
                                   const glm::vec3&  posOrDir,
                                   const glm::vec3&  color,
                                   float             intensity,
                                   float             range = 0.0f );

    // Destruye la luz referenciada por el handle e invalida su generacion.
    // Lanza SceneException(InvalidHandle) si el handle nunca fue valido.
    // Lanza SceneException(StaleHandle)   si la luz ya fue destruida antes.
    void destroyLight( LightEntityHandle& handle );


    // --- Main light (shadow caster) -----------------------------------------

    // Designa la luz que genera el shadow map.
    // Solo acepta luces de tipo Directional; lanza std::invalid_argument si no.
    // Lanza SceneException(InvalidHandle) si el handle nunca fue valido.
    // Lanza SceneException(StaleHandle)   si la luz ya fue destruida.
    void setMainLight(const LightEntityHandle& handle);

    // Retira la designacion sin destruir la luz.
    void clearMainLight();

    // Devuelve true si hay una main light designada y sigue viva.
    bool hasMainLight() const;


    // --- Operaciones globales -----------------------------------------------

    // Destruye todas las entidades y luces y reinicia la escena.
    void clear();

    // --- Consultas generales ------------------------------------------------

    bool        hasEntity( const RenderEntityHandle& handle ) const;
    std::size_t entityCount() const;

    bool        hasLight( const LightEntityHandle& handle ) const;
    std::size_t lightCount() const;

private:

    // --- Slots de entidades renderizables (privados) ------------------------

    struct EntitySlot {
        uint32_t     generation  = 1;
        bool         occupied    = false;
        bool         active      = true;
        bool         visible     = true;
        RenderObject renderObject;
    };

    // --- Slots de luces (privados) ------------------------------------------

    struct LightSlot {
        uint32_t    generation = 1;
        bool        occupied   = false;
        bool        active     = true;
        LightObject light;
    };

    // Par (index, generation) que identifica al shadow caster en _lightSlots.
    // std::optional vacio == sin main light designada.
    struct MainLightRef {
        uint32_t index = 0;
        uint32_t generation = 0;
    };

    std::optional<MainLightRef> _mainLightRef;


    // --- Acceso interno a slots de entidades (RenderEntityHandle) -----------

    friend class RenderEntityHandle;

    EntitySlot&       requireSlot     ( uint32_t index, uint32_t generation, const char* callSite );
    const EntitySlot& requireSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const;


    // --- Acceso interno a slots de luces (LightEntityHandle) ----------------

    friend class LightEntityHandle;

    LightSlot&       requireLightSlot     ( uint32_t index, uint32_t generation, const char* callSite );
    const LightSlot& requireLightSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const;


    // --- Render queue, light queue y main light (RenderEngine) --------------

    friend class RenderEngine;

    const std::vector<RenderObject>& buildRenderQueue() const;
    const std::vector<LightObject>& buildLightQueue()  const;

    // Devuelve el LightObject del shadow caster si sigue vivo y activo, nullptr si no.
    // RenderEngine lo llama cada frame para construir la VP de la shadow pass.
    // Devuelve nullptr si no hay main light o fue destruida: shadow pass se omite.
    const LightObject* tryGetMainLight() const;


    // --- Implementacion interna ---------------------------------------------

    bool hasEntity( uint32_t index, uint32_t generation ) const;
    bool hasLight ( uint32_t index, uint32_t generation ) const;

    EntitySlot*       tryGetSlot     ( uint32_t index, uint32_t generation );
    const EntitySlot* tryGetSlot     ( uint32_t index, uint32_t generation ) const;
    LightSlot*        tryGetLightSlot( uint32_t index, uint32_t generation );
    const LightSlot*  tryGetLightSlot( uint32_t index, uint32_t generation ) const;

    bool     isSlotIndexInRange ( uint32_t index ) const;
    bool     isLightIndexInRange( uint32_t index ) const;
    uint32_t getSlotGeneration      ( uint32_t index ) const;
    uint32_t getLightSlotGeneration ( uint32_t index ) const;

    uint32_t allocateSlot();
    uint32_t allocateLightSlot();

    static void bumpGeneration( uint32_t& generation );

private:
    std::vector<EntitySlot>           _slots;
    std::vector<uint32_t>             _freeSlots;
    mutable std::vector<RenderObject> _renderQueueCache;

    std::vector<LightSlot>           _lightSlots;
    std::vector<uint32_t>            _freeLightSlots;
    mutable std::vector<LightObject> _lightQueueCache;
};
