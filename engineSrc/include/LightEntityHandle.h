#pragma once

/**
 * @file LightEntityHandle.h
 * @brief Token de acceso a una luz en la Scene.
 */

#include <cstdint>
#include <string>

#include "BufferObjectsData.h"
#include "Scene.h"

// ---------------------------------------------------------------------------
// LightEntityHandle
// ---------------------------------------------------------------------------

/**
 * @brief Token unico que la aplicacion recibe al crear una luz en la Scene.
 *
 * Encapsula el par (index, generation) que identifica un LightSlot en la
 * Scene, mas un puntero a la Scene que lo emitio. Toda mutacion y consulta
 * se despacha directamente al slot: el slot es la fuente de verdad, el handle
 * es solo un punto de acceso validado.
 *
 * A diferencia de RenderEntityHandle, el handle NO almacena ninguna copia
 * local del estado de la luz. Los campos de LightObject son exactamente los
 * datos que la aplicacion escribe y el renderer lee.
 *
 * El handle es move-only.
 *
 * @note Al salir de scope, el handle NO destruye la luz. El caller decide
 *       cuando llamar a Scene::destroyLight().
 *
 * Ejemplo de uso tipico:
 * @code
 * LightEntityHandle sun = scene.createLight(
 *     LightType::Directional,
 *     glm::normalize( glm::vec3( -1, -1, 0 ) ),
 *     glm::vec3( 1, 1, 0.9f ),
 *     2.0f );
 * sun.setIntensity( 1.5f );
 * sun.setActive( false );
 * @endcode
 *
 * @see Scene::createLight()
 * @see Scene::destroyLight()
 */
class LightEntityHandle {
public:
    // --- Construccion y ciclo de vida del handle ----------------------------

    /// @brief Construye un handle invalido. No esta asociado a ninguna luz.
    LightEntityHandle() = default;

    LightEntityHandle( const LightEntityHandle& )            = delete;
    LightEntityHandle& operator=( const LightEntityHandle& ) = delete;

    /**
     * @brief Constructor de movimiento. Transfiere la propiedad del handle.
     * @param other Handle origen. Queda invalido tras el movimiento.
     */
    LightEntityHandle( LightEntityHandle&& other ) noexcept;

    /**
     * @brief Asignacion de movimiento. Transfiere la propiedad del handle.
     * @param other Handle origen. Queda invalido tras el movimiento.
     * @return Referencia a este handle.
     */
    LightEntityHandle& operator=( LightEntityHandle&& other ) noexcept;

    ~LightEntityHandle() = default;

    /**
     * @brief Indica si el handle fue emitido y la luz no ha sido destruida.
     *
     * No lanza excepciones. Un handle default-construido o movido devuelve @c false.
     *
     * @return @c true si el handle es valido y la luz existe en la Scene.
     */
    bool isValid() const;

    // --- Mutacion (despacha directamente al slot) ---------------------------

    /**
     * @defgroup light_setters Setters de propiedades de la luz
     *
     * Todos los metodos de mutacion lanzan @b SceneException(StaleHandle)
     * si la luz fue destruida con anterioridad.
     * @{
     */

    /**
     * @brief Establece la posicion o direccion de la luz.
     *
     * Para luces de tipo Point y Spotlight, este valor es la posicion en el
     * espacio del mundo. Para luces Directional, es la direccion normalizada
     * que apunta hacia la fuente.
     *
     * @param posOrDir Nueva posicion o direccion.
     */
    void setPosOrDir ( const glm::vec3& posOrDir  );

    /**
     * @brief Establece el color de la luz.
     * @param color Color RGB de la luz (valores tipicamente en [0, 1]).
     */
    void setColor    ( const glm::vec3& color      );

    /**
     * @brief Establece la intensidad de la luz.
     * @param intensity Intensidad de la luz (0 = apagada, valores mayores = mas brillante).
     */
    void setIntensity( float intensity             );

    /**
     * @brief Establece el rango de influencia de la luz.
     *
     * Solo es relevante para luces de tipo Point y Spotlight. En luces
     * Directional, este valor se ignora.
     *
     * @param range Distancia maxima de influencia en unidades de mundo.
     */
    void setRange    ( float range                 );

    /**
     * @brief Cambia el tipo de la luz.
     * @param type Nuevo tipo de luz (Directional, Point o Spotlight).
     * @see LightType
     */
    void setType     ( LightType type              );

    /**
     * @brief Activa o desactiva la luz.
     *
     * Una luz inactiva no contribuye a la iluminacion de la escena aunque
     * siga existiendo en la Scene.
     *
     * @param active @c true para activar, @c false para desactivar.
     */
    void setActive   ( bool active                 );

    /** @} */

    // --- Consulta (despacha directamente al slot) ---------------------------

    /**
     * @defgroup light_getters Getters de propiedades de la luz
     *
     * Todos los metodos de consulta lanzan @b SceneException(StaleHandle)
     * si la luz fue destruida con anterioridad.
     * @{
     */

    /**
     * @brief Devuelve la posicion (Point/Spot) o direccion (Directional) de la luz.
     * @return Posicion o direccion en espacio del mundo.
     */
    glm::vec3 getPosOrDir()  const;

    /// @brief Devuelve el color RGB de la luz.
    glm::vec3 getColor()     const;

    /// @brief Devuelve la intensidad de la luz.
    float     getIntensity() const;

    /// @brief Devuelve el rango de influencia de la luz.
    float     getRange()     const;

    /// @brief Devuelve el tipo de la luz.
    LightType getType()      const;

    /// @brief Indica si la luz esta activa.
    bool      isActive()     const;

    /** @} */

private:
    friend class Scene;

    LightEntityHandle( uint32_t index, uint32_t generation, Scene* scene )
        : _index( index ), _generation( generation ), _scene( scene ) {
    }

    void invalidate();

private:
    uint32_t  _index      = 0;
    uint32_t  _generation = 0;   ///< 0 == handle nunca inicializado.
    Scene*    _scene      = nullptr;
};
