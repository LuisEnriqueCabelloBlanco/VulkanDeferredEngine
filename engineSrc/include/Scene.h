#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "BufferObjectsData.h"
#include "Camera.h"
#include "CameraHandle.h"
#include "ResourceLimits.h"
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

static constexpr uint32_t MAX_ENTITIES = ResourceLimits::MAX_ENTITIES;
static constexpr uint32_t MAX_LIGHTS   = ResourceLimits::MAX_LIGHTS;

class RenderEntityHandle;
class LightEntityHandle;

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

/*
 Contenedor de entidades renderizables, luces y la camara principal.

La aplicacion crea y destruye entidades a traves de RenderEntityHandle y
luces a traves de LightEntityHandle, emitidos exclusivamente por Scene.

La aplicacion interactua con la camara a traves del CameraHandle obtenido
con getCamera(). La camara se inicializa con valores por defecto y
RenderEngine ajusta su aspect ratio en cada resize.
*/
class Scene {
public:
    Scene();
    ~Scene() = default;

    Scene( const Scene& ) = delete;
    Scene& operator=( const Scene& ) = delete;
    Scene( Scene&& ) = delete;
    Scene& operator=( Scene&& ) = delete;


    // -------------------------------------------------------------------------
    // Camara
    // -------------------------------------------------------------------------

    // Devuelve el handle de la camara principal. Hay una y solo una camara
    // por escena; el handle tiene el mismo ciclo de vida que Scene.
    CameraHandle& getCamera() { return _cameraHandle; }


    // -------------------------------------------------------------------------
    // Ciclo de vida de entidades renderizables
    // -------------------------------------------------------------------------

    // Crea una entidad vacia y devuelve su handle.
    RenderEntityHandle createEntity();

    // Crea una entidad con mesh, material y transform ya asignados.
    RenderEntityHandle createEntity(MeshHandle     mesh,
        MaterialHandle material,
        const Transform& transform);

    // Destruye la entidad e invalida el handle.
    // Lanza SceneException(InvalidHandle) si el handle nunca fue valido.
    // Lanza SceneException(StaleHandle)   si la entidad ya fue destruida.
    void destroyEntity(RenderEntityHandle& handle);
    

    // -------------------------------------------------------------------------
    // Ciclo de vida de luces
    // -------------------------------------------------------------------------

    // Crea una luz y devuelve su handle.
    // posOrDir: posicion para Point/Spotlight, direccion normalizada para Directional.
    // range:    solo relevante para Point y Spotlight.
    LightEntityHandle createLight( LightType         type,
                                   const glm::vec3&  posOrDir,
                                   const glm::vec3&  color,
                                   float             intensity,
                                   float             range = 0.0f );

    // Destruye la luz e invalida el handle.
    // Lanza SceneException(InvalidHandle) si el handle nunca fue valido.
    // Lanza SceneException(StaleHandle)   si la luz ya fue destruida antes.
    void destroyLight( LightEntityHandle& handle );


    // -------------------------------------------------------------------------
    // Main light (shadow caster)
    // -------------------------------------------------------------------------
    
    // Designa la luz que genera el shadow map.
    // Solo acepta luces de tipo Directional; lanza std::invalid_argument si no.
    // Lanza SceneException(InvalidHandle) si el handle nunca fue valido.
    // Lanza SceneException(StaleHandle)   si la luz ya fue destruida.
    void setMainLight(const LightEntityHandle& handle);

    // Retira la designacion sin destruir la luz.
    void clearMainLight();

    // Devuelve true si hay una main light designada y sigue viva.
    bool hasMainLight() const;


    // -------------------------------------------------------------------------
    // Operaciones globales
    // -------------------------------------------------------------------------

    // Destruye todas las entidades y luces. La camara no se resetea.
    void clear();


    // -------------------------------------------------------------------------
    // Consultas
    // -------------------------------------------------------------------------

    bool        hasEntity(const RenderEntityHandle& handle) const;
    std::size_t entityCount() const;
    bool        hasLight(const LightEntityHandle& handle)  const;
    std::size_t lightCount() const;


private:

    // -------------------------------------------------------------------------
    // Slots de entidades
    // -------------------------------------------------------------------------

    struct EntitySlot {
        uint32_t     generation  = 1;
        bool         occupied    = false;
        bool         active      = true;
        bool         visible     = true;
        RenderObject renderObject;
    };


    // -------------------------------------------------------------------------
    // Slots de luces
    // -------------------------------------------------------------------------

    struct LightSlot {
        uint32_t    generation = 1;
        bool        occupied   = false;
        bool        active     = true;
        LightObject light;
    };

    // Par (index, generation) que identifica al shadow caster en _lightSlots.  
    struct MainLightRef {
        uint32_t index = 0;
        uint32_t generation = 0;
    };
    // optional vacio == sin main light designada.
    std::optional<MainLightRef> _mainLightRef;


    // -------------------------------------------------------------------------
    // Acceso interno — RenderEntityHandle
    // -------------------------------------------------------------------------
    friend class RenderEntityHandle;

    EntitySlot&       requireSlot     ( uint32_t index, uint32_t generation, const char* callSite );
    const EntitySlot& requireSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const;


    // -------------------------------------------------------------------------
    // Acceso interno — LightEntityHandle
    // -------------------------------------------------------------------------
    friend class LightEntityHandle;

    LightSlot&       requireLightSlot     ( uint32_t index, uint32_t generation, const char* callSite );
    const LightSlot& requireLightSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const;


    // -------------------------------------------------------------------------
    // Acceso interno — RenderEngine
    // -------------------------------------------------------------------------
    friend class RenderEngine;

    // Construye y devuelve la render queue del frame actual.
    const std::vector<RenderObject>& buildRenderQueue() const;

    // Construye y devuelve la light queue del frame actual.
    const std::vector<LightObject>& buildLightQueue()  const;

    // Devuelve el LightObject del shadow caster si sigue vivo y activo, nullptr si no.
    // RenderEngine lo llama cada frame para construir la VP de la shadow pass.
    // Devuelve nullptr si no hay main light o fue destruida: shadow pass se omite.
    const LightObject* tryGetMainLight() const;

    // Acceso directo a la Camera para leer matrices VP cada frame.
    // RenderEngine nunca muta la camara a traves de este puntero;
    // solo lee getProjMatrix() / getViewMatrix() / getPosition().
    const Camera& getInternalCamera() const { return _camera; }

    // RenderEngine actualiza el aspect ratio en cada resize.
    void setCameraAspectRatio(float aspectRatio) { _camera.setAspectRatio(aspectRatio); }


    // -------------------------------------------------------------------------
    // Implementacion interna
    // -------------------------------------------------------------------------

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


    // -------------------------------------------------------------------------
    // Datos
    // -------------------------------------------------------------------------

    Camera       _camera;
    CameraHandle _cameraHandle;

    std::vector<EntitySlot>           _slots;
    std::vector<uint32_t>             _freeSlots;
    mutable std::vector<RenderObject> _renderQueueCache;

    std::vector<LightSlot>           _lightSlots;
    std::vector<uint32_t>            _freeLightSlots;
    mutable std::vector<LightObject> _lightQueueCache;

};
