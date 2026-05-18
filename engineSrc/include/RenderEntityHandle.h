#pragma once

/**
 * @file RenderEntityHandle.h
 * @brief Token de acceso a una entidad renderizable en la Scene.
 */

#include <cstdint>
#include <string>

#include "Scene.h"
#include "ResourceHandles.h"
#include "Transform.h"

// ---------------------------------------------------------------------------
// RenderEntityHandle
// ---------------------------------------------------------------------------

/**
 * @brief Token unico que la aplicacion recibe al crear una entidad renderizable.
 *
 * Encapsula el par (index, generation) que identifica un slot en la Scene,
 * mas un puntero a la Scene que lo emitio. Toda mutacion y consulta se
 * despacha a traves del handle, sin que el caller necesite acceder a Scene
 * directamente.
 *
 * El handle es move-only: un handle, una entidad, un objeto renderizable.
 * Copiar handles esta prohibido para evitar comportamientos indeterminados.
 *
 * El handle almacena un Transform local. Cualquier mutacion sobre el transform
 * actualiza automaticamente la model matrix del slot correspondiente en la Scene.
 *
 * @note Al salir de scope, el handle NO destruye la entidad. El caller decide
 *       cuando llamar a Scene::destroyEntity().
 *
 * Ejemplo de uso tipico:
 * @code
 * RenderEntityHandle entity = scene.createEntity(
 *     mesh, material,
 *     Transform{ { 0.f, 1.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f } }
 * );
 * entity.translate( { 0.f, 1.f, 0.f } );
 * entity.setVisible( false );
 * @endcode
 *
 * @see Scene::createEntity()
 * @see Scene::destroyEntity()
 */
class RenderEntityHandle {
public:
    // --- Construccion y ciclo de vida del handle ----------------------------

    /// @brief Construye un handle invalido. No esta asociado a ninguna entidad.
    RenderEntityHandle() = default;

    RenderEntityHandle( const RenderEntityHandle& )            = delete;
    RenderEntityHandle& operator=( const RenderEntityHandle& ) = delete;

    /**
     * @brief Constructor de movimiento. Transfiere la propiedad del handle.
     * @param other Handle origen. Queda invalido tras el movimiento.
     */
    RenderEntityHandle( RenderEntityHandle&& other ) noexcept;

    /**
     * @brief Asignacion de movimiento. Transfiere la propiedad del handle.
     * @param other Handle origen. Queda invalido tras el movimiento.
     * @return Referencia a este handle.
     */
    RenderEntityHandle& operator=( RenderEntityHandle&& other ) noexcept;

    ~RenderEntityHandle() = default;

    /**
     * @brief Indica si el handle fue emitido por su Scene y la entidad sigue viva.
     *
     * No lanza excepciones. Un handle default-construido o movido devuelve @c false.
     *
     * @return @c true si el handle es valido y la entidad existe en la Scene.
     */
    bool isValid() const;

    // --- Transform: setters (override) --------------------------------------

    /**
     * @defgroup transform_setters Setters absolutos de transformacion
     *
     * Contrato de errores para todos los metodos de transform:
     * - @b SceneException(InvalidHandle): handle no inicializado.
     * - @b SceneException(StaleHandle): entidad destruida o invalidada.
     * @{
     */

    /**
     * @brief Establece la posicion absoluta de la entidad en el espacio del mundo.
     * @param position Nueva posicion.
     */
    void setPosition( const glm::vec3& position );

    /**
     * @brief Establece la rotacion absoluta en radianes, orden XYZ (pitch, yaw, roll).
     * @param rotationRadians Rotacion en radianes.
     */
    void setRotation( const glm::vec3& rotationRadians );

    /**
     * @brief Establece la escala absoluta por eje.
     * @param scale Factor de escala en X, Y y Z.
     */
    void setScale   ( const glm::vec3& scale );

    /**
     * @brief Establece una escala uniforme en los tres ejes.
     * @param uniformScale Factor de escala.
     */
    void setScale   ( float uniformScale );

    /**
     * @brief Reemplaza el transform completo de la entidad.
     * @param transform Nuevo transform.
     */
    void setTransform( const Transform& transform );

    /**
     * @brief Reemplaza el transform completo especificando sus componentes.
     * @param position        Nueva posicion.
     * @param rotationRadians Nueva rotacion en radianes (orden XYZ).
     * @param scale           Nueva escala (por defecto 1, 1, 1).
     */
    void setTransform( const glm::vec3& position,
                       const glm::vec3& rotationRadians,
                       const glm::vec3& scale = glm::vec3( 1.0f ) );

    /** @} */

    // --- Transform: mutaciones relativas ------------------------------------

    /**
     * @brief Desplaza la entidad sumando un vector delta en espacio del mundo.
     * @param delta Desplazamiento a aplicar.
     */
    void translate( const glm::vec3& delta );

    /**
     * @brief Rota la entidad incrementalmente en radianes, orden XYZ.
     * @param deltaRadians Incremento de rotacion (pitch, yaw, roll).
     */
    void rotate   ( const glm::vec3& deltaRadians );

    /**
     * @brief Rota la entidad alrededor del eje X (pitch).
     * @param radians Incremento en radianes.
     */
    void rotateX  ( float radians );

    /**
     * @brief Rota la entidad alrededor del eje Y (yaw).
     * @param radians Incremento en radianes.
     */
    void rotateY  ( float radians );

    /**
     * @brief Rota la entidad alrededor del eje Z (roll).
     * @param radians Incremento en radianes.
     */
    void rotateZ  ( float radians );

    /**
     * @brief Escala la entidad multiplicando el factor actual por un vector de factores.
     * @param factor Factores multiplicativos por eje.
     */
    void scale    ( const glm::vec3& factor );

    /**
     * @brief Escala la entidad multiplicando el factor actual por un factor uniforme.
     * @param uniformFactor Factor multiplicativo aplicado a los tres ejes.
     */
    void scale    ( float uniformFactor );

    /**
     * @brief Resetea el transform: posicion al origen, sin rotacion, escala 1.
     */
    void resetTransform();

    // --- Transform: getters -------------------------------------------------

    /// @brief Devuelve la posicion actual de la entidad.
    const glm::vec3& getPosition()    const { return _transform.getPosition(); }

    /// @brief Devuelve la rotacion actual en radianes, orden XYZ.
    const glm::vec3& getRotation()    const { return _transform.getRotation(); }

    /// @brief Devuelve la escala actual por eje.
    const glm::vec3& getScale()       const { return _transform.getScale(); }

    // --- Mesh y material ----------------------------------------------------

    /**
     * @defgroup mesh_material Gestion de mesh y material
     *
     * Contrato de errores para todos los metodos de mesh y material:
     * - @b SceneException(InvalidHandle): handle no inicializado.
     * - @b SceneException(StaleHandle): entidad destruida o invalidada.
     * @{
     */

    /**
     * @brief Asigna una malla a la entidad.
     * @param mesh Handle valido de una malla en el ResourceManager.
     */
    void setMesh    ( MeshHandle mesh );

    /**
     * @brief Asigna un material a la entidad.
     * @param material Handle valido de un material en el ResourceManager.
     */
    void setMaterial( MaterialHandle material );

    /// @brief Elimina la malla asignada. La entidad no se renderizara sin malla.
    void clearMesh();

    /// @brief Elimina el material asignado.
    void clearMaterial();

    /// @brief Devuelve el handle de la malla asignada (puede ser invalido si no hay malla).
    MeshHandle     getMesh()     const;

    /// @brief Devuelve el handle del material asignado (puede ser invalido si no hay material).
    MaterialHandle getMaterial() const;

    /// @brief Indica si la entidad tiene una malla asignada.
    bool           hasMesh()     const;

    /// @brief Indica si la entidad tiene un material asignado.
    bool           hasMaterial() const;

    /** @} */

    // --- Flags de estado ----------------------------------------------------

    /**
     * @defgroup entity_flags Flags de visibilidad y activacion
     *
     * Contrato de errores para todos los metodos de flags:
     * - @b SceneException(InvalidHandle): handle no inicializado.
     * - @b SceneException(StaleHandle): entidad destruida o invalidada.
     * @{
     */

    /**
     * @brief Activa o desactiva la entidad.
     *
     * Una entidad inactiva no se incluye en el render queue ni en el proceso de culling.
     *
     * @param active @c true para activar, @c false para desactivar.
     */
    void setActive ( bool active  );

    /**
     * @brief Establece la visibilidad de la entidad.
     *
     * Una entidad no visible no se envia al renderer aunque este activa.
     *
     * @param visible @c true para hacerla visible, @c false para ocultarla.
     */
    void setVisible( bool visible );

    /// @brief Indica si la entidad esta activa.
    bool isActive()  const;

    /// @brief Indica si la entidad es visible.
    bool isVisible() const;

    /** @} */

private:
    friend class Scene;

    RenderEntityHandle( uint32_t index, uint32_t generation, Scene* scene )
        : _index( index ), _generation( generation ), _scene( scene ) {
    }

    void invalidate();

    /// Escribe la model matrix actual del transform en el slot de la Scene.
    void flushModelMatrix();

private:
    uint32_t  _index      = 0;
    uint32_t  _generation = 0;   ///< 0 == handle nunca inicializado.
    Scene*    _scene      = nullptr;
    Transform _transform;
};
