#pragma once

/**
 * @file Scene.h
 * @brief Contenedor de entidades renderizables, luces y la camara principal.
 */

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

/**
 * @brief Codigos de error del dominio de Scene.
 *
 * Permiten distinguir el motivo exacto del fallo sin depender del mensaje.
 *
 * @see SceneException
 */
enum class SceneErrorCode {
    InvalidHandle, ///< El handle no fue emitido por este Scene o nunca fue valido.
    StaleHandle,   ///< El handle era valido pero la entidad fue destruida posteriormente.
    LimitExceeded, ///< Se ha superado el numero maximo de entidades o luces permitidas.
};

/**
 * @brief Excepcion lanzada por Scene ante operaciones invalidas sobre handles.
 *
 * Hereda de std::runtime_error. Ademas del mensaje de texto, expone el
 * codigo de error para que el caller pueda reaccionar de forma programatica.
 *
 * @see SceneErrorCode
 */
class SceneException : public std::runtime_error {
public:
    /**
     * @brief Construye la excepcion con un codigo y un mensaje descriptivo.
     * @param code    Codigo que identifica la causa del fallo.
     * @param message Descripcion legible del error.
     */
    SceneException( SceneErrorCode code, const std::string& message )
        : std::runtime_error( message ), _code( code ) {
    }

    /**
     * @brief Devuelve el codigo de error asociado a la excepcion.
     * @return Codigo que identifica la causa del fallo.
     */
    SceneErrorCode code() const { return _code; }

private:
    SceneErrorCode _code;
};

// ---------------------------------------------------------------------------

class RenderEntityHandle;
class LightEntityHandle;

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

/**
 * @brief Contenedor de entidades renderizables, luces y la camara principal.
 *
 * La aplicacion crea y destruye entidades a traves de RenderEntityHandle y
 * luces a traves de LightEntityHandle, emitidos exclusivamente por Scene.
 *
 * La aplicacion interactua con la camara a traves del CameraHandle obtenido
 * con getCamera(). La camara se inicializa con valores por defecto y
 * RenderEngine ajusta su aspect ratio en cada resize.
 *
 * Se obtiene a traves de EngineAPI::getScene().
 *
 * Limites de capacidad:
 * - Entidades: ResourceLimits::MAX_ENTITIES
 * - Luces:     ResourceLimits::MAX_LIGHTS
 *
 * @see EngineAPI::getScene()
 * @see RenderEntityHandle
 * @see LightEntityHandle
 * @see CameraHandle
 */
class Scene {
public:
    Scene();
    ~Scene() = default;

    Scene( const Scene& )            = delete;
    Scene& operator=( const Scene& ) = delete;
    Scene( Scene&& )                 = delete;
    Scene& operator=( Scene&& )      = delete;


    // -------------------------------------------------------------------------
    // Camara
    // -------------------------------------------------------------------------

    /**
     * @brief Devuelve el handle de la camara principal de la escena.
     *
     * Hay exactamente una camara por escena. El handle tiene el mismo ciclo de
     * vida que Scene y no puede ser almacenado mas alla de la vida del motor.
     *
     * @return Referencia al CameraHandle de la camara principal.
     * @see CameraHandle
     */
    CameraHandle& getCamera() { return _cameraHandle; }


    // -------------------------------------------------------------------------
    // Ciclo de vida de entidades renderizables
    // -------------------------------------------------------------------------

    /**
     * @brief Crea una entidad vacia sin mesh, material ni transform especificos.
     *
     * La entidad se crea activa y visible. El transform se inicializa en el
     * origen con escala unitaria.
     *
     * @return Handle move-only que identifica la entidad creada.
     * @throws SceneException(LimitExceeded) si se supera MAX_ENTITIES.
     * @see ResourceLimits::MAX_ENTITIES
     */
    [[nodiscard]] RenderEntityHandle createEntity();

    /**
     * @brief Crea una entidad con mesh, material y transform ya asignados.
     *
     * @param mesh      Handle de la malla a asignar (puede ser invalido).
     * @param material  Handle del material a asignar (puede ser invalido).
     * @param transform Transform inicial de la entidad en el espacio del mundo.
     * @return Handle move-only que identifica la entidad creada.
     * @throws SceneException(LimitExceeded) si se supera MAX_ENTITIES.
     */
    [[nodiscard]] RenderEntityHandle createEntity( MeshHandle       mesh,
                                                   MaterialHandle   material,
                                                   const Transform& transform );

    /**
     * @brief Destruye la entidad asociada al handle e invalida el handle.
     *
     * Tras la llamada, el handle queda stale y cualquier acceso posterior
     * a sus metodos lanzara SceneException(StaleHandle).
     *
     * @param handle Handle de la entidad a destruir.
     * @throws SceneException(InvalidHandle) si el handle nunca fue valido.
     * @throws SceneException(StaleHandle)   si la entidad ya fue destruida.
     */
    void destroyEntity( RenderEntityHandle& handle );


    // -------------------------------------------------------------------------
    // Ciclo de vida de luces
    // -------------------------------------------------------------------------

    /**
     * @brief Crea una luz y devuelve su handle.
     *
     * @param type      Tipo de luz (Directional, Point o Spotlight).
     * @param posOrDir  Posicion en espacio del mundo (Point/Spot) o direccion
     *                  normalizada hacia la fuente (Directional).
     * @param color     Color RGB de la luz.
     * @param intensity Intensidad de la luz.
     * @param range     Radio de influencia en unidades de mundo. Solo relevante
     *                  para Point y Spotlight (default: 0).
     * @return Handle move-only que identifica la luz creada.
     * @throws SceneException(LimitExceeded) si se supera MAX_LIGHTS.
     * @see ResourceLimits::MAX_LIGHTS
     * @see LightType
     */
    [[nodiscard]] LightEntityHandle createLight( LightType        type,
                                                 const glm::vec3& posOrDir,
                                                 const glm::vec3& color,
                                                 float            intensity,
                                                 float            range = 0.0f );

    /**
     * @brief Destruye la luz asociada al handle e invalida el handle.
     *
     * @param handle Handle de la luz a destruir.
     * @throws SceneException(InvalidHandle) si el handle nunca fue valido.
     * @throws SceneException(StaleHandle)   si la luz ya fue destruida.
     */
    void destroyLight( LightEntityHandle& handle );


    // -------------------------------------------------------------------------
    // Main light (shadow caster)
    // -------------------------------------------------------------------------

    /**
     * @brief Designa la luz que genera el shadow map.
     *
     * Solo acepta luces de tipo Directional. Si se designa una luz Point o
     * Spotlight se lanza std::invalid_argument.
     *
     * Solo puede haber una main light activa simultaneamente. Llamar a este
     * metodo sustituye cualquier designacion anterior.
     *
     * @param handle Handle de una luz Directional viva.
     * @throws std::invalid_argument         si la luz no es de tipo Directional.
     * @throws SceneException(InvalidHandle) si el handle nunca fue valido.
     * @throws SceneException(StaleHandle)   si la luz ya fue destruida.
     */
    void setMainLight( const LightEntityHandle& handle );

    /**
     * @brief Retira la designacion de main light sin destruir la luz.
     *
     * Tras la llamada no hay sombra principal hasta que se designe otra luz.
     */
    void clearMainLight();

    /**
     * @brief Indica si hay una main light designada y sigue viva.
     * @return @c true si existe una main light activa.
     */
    bool hasMainLight() const;


    // -------------------------------------------------------------------------
    // Operaciones globales
    // -------------------------------------------------------------------------

    /**
     * @brief Destruye todas las entidades y luces de la escena.
     *
     * La designacion de main light tambien se elimina. La camara no se resetea
     * y mantiene su posicion, orientacion y parametros de proyeccion actuales.
     */
    void clear();


    // -------------------------------------------------------------------------
    // Consultas
    // -------------------------------------------------------------------------

    /**
     * @brief Indica si la entidad asociada al handle sigue viva en la escena.
     * @param handle Handle de la entidad a consultar.
     * @return @c true si la entidad existe y el handle es valido.
     */
    bool        hasEntity( const RenderEntityHandle& handle ) const;

    /// @brief Devuelve el numero de entidades actualmente activas en la escena.
    std::size_t entityCount() const;

    /**
     * @brief Indica si la luz asociada al handle sigue viva en la escena.
     * @param handle Handle de la luz a consultar.
     * @return @c true si la luz existe y el handle es valido.
     */
    bool        hasLight( const LightEntityHandle& handle ) const;

    /// @brief Devuelve el numero de luces actualmente activas en la escena.
    std::size_t lightCount() const;

    /**
     * @brief Itera sobre todas las entidades vivas y llama a @p fn por cada una.
     *
     * La funcion recibe un RenderEntityHandle por referencia construido
     * internamente para el slot correspondiente.
     *
     * @warning El comportamiento es indefinido si @p fn crea o destruye
     *          entidades durante la enumeracion, o si invalida handles de
     *          entidades aun no visitadas.
     *
     * @tparam Fn Callable con firma compatible con @c void(RenderEntityHandle&).
     * @param  fn Funcion a invocar por cada entidad viva.
     *
     * Ejemplo:
     * @code
     * scene.forEachEntity( [&]( RenderEntityHandle& e ) {
     *     if (e.isValid()) std::cout << e.getPosition().x << "\n";
     * });
     * @endcode
     */
    template <typename Fn>
    void forEachEntity( Fn&& fn )
    {
        for ( uint32_t i = 0; i < static_cast<uint32_t>( _slots.size() ); ++i ) {
            EntitySlot& slot = _slots[i];
            if ( !slot.occupied ) continue;
            RenderEntityHandle handle{ i, slot.generation, this };
            fn( handle );
        }
    }

    /**
     * @brief Itera sobre todas las luces vivas y llama a @p fn por cada una.
     *
     * @warning El comportamiento es indefinido si @p fn crea o destruye
     *          luces durante la enumeracion, o si invalida handles de
     *          luces aun no visitadas.
     *
     * @tparam Fn Callable con firma compatible con @c void(LightEntityHandle&).
     * @param  fn Funcion a invocar por cada luz viva.
     *
     * Ejemplo:
     * @code
     * scene.forEachLight( [&]( LightEntityHandle& l ) {
     *     if (l.isValid()) std::cout << l.getIntensity() << "\n";
     * });
     * @endcode
     */
    template <typename Fn>
    void forEachLight( Fn&& fn )
    {
        for ( uint32_t i = 0; i < static_cast<uint32_t>( _lightSlots.size() ); ++i ) {
            LightSlot& slot = _lightSlots[i];
            if ( !slot.occupied ) continue;
            LightEntityHandle handle{ i, slot.generation, this };
            fn( handle );
        }
    }


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

    struct MainLightRef {
        uint32_t index      = 0;
        uint32_t generation = 0;
    };
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

    int getMainLightIndexInQueue();
    const std::vector<RenderObject>& buildRenderQueue() const;
    const std::vector<LightObject>&  buildLightQueue()  const;
    const LightObject*               tryGetMainLight()  const;

    const Camera& getInternalCamera() const { return _camera; }
    void setCameraAspectRatio( float aspectRatio ) { _camera.setAspectRatio( aspectRatio ); }


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
    uint32_t getSlotGeneration     ( uint32_t index ) const;
    uint32_t getLightSlotGeneration( uint32_t index ) const;

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
    mutable int _mainLightIndex;
};
