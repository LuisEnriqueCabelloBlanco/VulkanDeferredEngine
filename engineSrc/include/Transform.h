#pragma once

/**
 * @file Transform.h
 * @brief Transformacion de posicion, rotacion y escala de un objeto en el mundo.
 */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class RenderEntityHandle;

// ---------------------------------------------------------------------------
// Transform
// ---------------------------------------------------------------------------

/**
 * @brief Almacena la posicion, rotacion (en radianes, orden XYZ) y escala de
 *        un objeto en el espacio del mundo.
 *
 * Es un valor plano sin jerarquia ni nodo padre. La model matrix se recalcula
 * de forma lazy al solicitarla y se invalida automaticamente ante cualquier
 * mutacion: en cadenas de operaciones del tipo translate + rotate dentro del
 * mismo frame, el coste de reconstruccion es unico.
 *
 * Convencion de rotacion: Euler XYZ aplicado en ese orden (pitch, yaw, roll).
 *
 * Ejemplo de uso:
 * @code
 * Transform t(
 *     { 0.0f, 1.0f, 0.0f },   // posicion
 *     { 0.0f, 0.0f, 0.0f },   // rotacion en radianes
 *     { 1.0f, 1.0f, 1.0f }    // escala
 * );
 * t.translate( { 0.0f, 0.5f, 0.0f } );
 * t.rotateY( glm::radians( 45.0f ) );
 * @endcode
 */
class Transform {
public:
    /// @brief Construye un Transform en el origen, sin rotacion y con escala unitaria.
    Transform() = default;

    /**
     * @brief Construye un Transform con valores iniciales explicitos.
     * @param position        Posicion en el espacio del mundo.
     * @param rotationRadians Rotacion en radianes, orden XYZ (pitch, yaw, roll).
     * @param scale           Factor de escala en cada eje.
     */
    Transform( const glm::vec3& position,
               const glm::vec3& rotationRadians,
               const glm::vec3& scale )
        : _position( position )
        , _rotation( rotationRadians )
        , _scale( scale )
        , _dirty( true ) {
    }

    // --- Setters (override) -------------------------------------------------

    /**
     * @brief Establece la posicion absoluta en el espacio del mundo.
     * @param position Nueva posicion.
     */
    void setPosition( const glm::vec3& position ) {
        _position = position;
        _dirty = true;
    }

    /**
     * @brief Establece la rotacion absoluta en radianes, orden XYZ.
     * @param rotationRadians Rotacion en radianes (pitch X, yaw Y, roll Z).
     */
    void setRotation( const glm::vec3& rotationRadians ) {
        _rotation = rotationRadians;
        _dirty = true;
    }

    /**
     * @brief Establece la escala absoluta por eje.
     * @param scale Factor de escala en X, Y y Z.
     */
    void setScale( const glm::vec3& scale ) {
        _scale = scale;
        _dirty = true;
    }

    /**
     * @brief Establece una escala uniforme en los tres ejes.
     * @param uniformScale Factor de escala aplicado a X, Y y Z.
     */
    void setScale( float uniformScale ) {
        _scale = glm::vec3( uniformScale );
        _dirty = true;
    }

    // --- Mutaciones relativas -----------------------------------------------

    /**
     * @brief Desplaza la posicion sumando un vector delta.
     * @param delta Desplazamiento en el espacio del mundo.
     */
    void translate( const glm::vec3& delta ) {
        _position += delta;
        _dirty = true;
    }

    /**
     * @brief Rota incrementalmente sumando angulos en radianes en orden XYZ.
     * @param deltaRadians Incremento de rotacion en radianes (pitch, yaw, roll).
     */
    void rotate( const glm::vec3& deltaRadians ) {
        _rotation += deltaRadians;
        _dirty = true;
    }

    /**
     * @brief Rota alrededor del eje X (pitch).
     * @param radians Incremento de rotacion en radianes.
     */
    void rotateX( float radians ) { _rotation.x += radians; _dirty = true; }

    /**
     * @brief Rota alrededor del eje Y (yaw).
     * @param radians Incremento de rotacion en radianes.
     */
    void rotateY( float radians ) { _rotation.y += radians; _dirty = true; }

    /**
     * @brief Rota alrededor del eje Z (roll).
     * @param radians Incremento de rotacion en radianes.
     */
    void rotateZ( float radians ) { _rotation.z += radians; _dirty = true; }

    /**
     * @brief Escala multiplicando el factor actual por un vector de factores.
     * @param factor Factores multiplicativos por eje.
     */
    void scale( const glm::vec3& factor ) {
        _scale *= factor;
        _dirty = true;
    }

    /**
     * @brief Escala multiplicando el factor actual por un factor uniforme.
     * @param uniformFactor Factor multiplicativo aplicado a los tres ejes.
     */
    void scale( float uniformFactor ) {
        _scale *= uniformFactor;
        _dirty = true;
    }

    // --- Getters ------------------------------------------------------------

    /// @brief Devuelve la posicion actual en el espacio del mundo.
    const glm::vec3& getPosition() const { return _position; }

    /// @brief Devuelve la rotacion actual en radianes, orden XYZ.
    const glm::vec3& getRotation() const { return _rotation; }

    /// @brief Devuelve la escala actual por eje.
    const glm::vec3& getScale()    const { return _scale; }

    // --- Reset --------------------------------------------------------------

    /**
     * @brief Resetea el transform al estado inicial: origen, sin rotacion, escala 1.
     *
     * Equivale a construir un Transform por defecto.
     */
    void reset() {
        _position = glm::vec3( 0.0f );
        _rotation = glm::vec3( 0.0f );
        _scale    = glm::vec3( 1.0f );
        _dirty    = true;
    }

private:
    friend class RenderEntityHandle;

    /**
     * @brief Devuelve la model matrix actualizada.
     *
     * Si el transform no ha sido modificado desde la ultima llamada, devuelve
     * la matrix cacheada. Solo accesible por RenderEntityHandle.
     *
     * @return Referencia a la model matrix (TRS, orden: traslacion, rotacion XYZ, escala).
     */
    const glm::mat4& getModelMatrix() const {
        if (_dirty) {
            rebuildModelMatrix();
            _dirty = false;
        }
        return _modelMatrix;
    }

    void rebuildModelMatrix() const {
        glm::mat4 m{ 1.0f };
        m = glm::translate( m, _position );
        m = glm::rotate( m, _rotation.x, glm::vec3( 1.0f, 0.0f, 0.0f ) );
        m = glm::rotate( m, _rotation.y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
        m = glm::rotate( m, _rotation.z, glm::vec3( 0.0f, 0.0f, 1.0f ) );
        m = glm::scale( m, _scale );
        _modelMatrix = m;
    }

private:
    glm::vec3 _position{ 0.0f };
    glm::vec3 _rotation{ 0.0f };   ///< Radianes, orden XYZ.
    glm::vec3 _scale   { 1.0f };

    mutable glm::mat4 _modelMatrix{ 1.0f };
    mutable bool      _dirty = true;
};
