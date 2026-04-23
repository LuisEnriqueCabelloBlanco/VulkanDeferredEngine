#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class RenderEntityHandle;

// ---------------------------------------------------------------------------
// Transform
// ---------------------------------------------------------------------------

/*
Transform almacena posicion, rotacion (en radianes, orden XYZ) y escala de un
objeto en el espacio del mundo. Es un valor plano sin jerarquia ni padre.

La model matrix se recalcula de forma lazy al llamar a getModelMatrix(), y se
invalida automaticamente ante cualquier mutacion. Para mutaciones en cadena
(ej: translate + rotate en el mismo frame) el coste de reconstruccion es unico.

Convencion de rotacion: Euler XYZ aplicado en ese orden (pitch, yaw, roll).
*/
class Transform {
public:
    Transform() = default;

    Transform( const glm::vec3& position,
               const glm::vec3& rotationRadians,
               const glm::vec3& scale )
        : _position( position )
        , _rotation( rotationRadians )
        , _scale( scale )
        , _dirty( true ) {
    }

    // --- Setters (override) -------------------------------------------------

    void setPosition( const glm::vec3& position ) {
        _position = position;
        _dirty = true;
    }

    void setRotation( const glm::vec3& rotationRadians ) {
        _rotation = rotationRadians;
        _dirty = true;
    }

    void setScale( const glm::vec3& scale ) {
        _scale = scale;
        _dirty = true;
    }

    void setScale( float uniformScale ) {
        _scale = glm::vec3( uniformScale );
        _dirty = true;
    }

    // --- Mutaciones relativas -----------------------------------------------

    void translate( const glm::vec3& delta ) {
        _position += delta;
        _dirty = true;
    }

    void rotate( const glm::vec3& deltaRadians ) {
        _rotation += deltaRadians;
        _dirty = true;
    }

    void rotateX( float radians ) { _rotation.x += radians; _dirty = true; }
    void rotateY( float radians ) { _rotation.y += radians; _dirty = true; }
    void rotateZ( float radians ) { _rotation.z += radians; _dirty = true; }

    void scale( const glm::vec3& factor ) {
        _scale *= factor;
        _dirty = true;
    }

    void scale( float uniformFactor ) {
        _scale *= uniformFactor;
        _dirty = true;
    }

    // --- Getters ------------------------------------------------------------

    const glm::vec3& getPosition() const { return _position; }
    const glm::vec3& getRotation() const { return _rotation; }
    const glm::vec3& getScale()    const { return _scale; }

    // --- Reset --------------------------------------------------------------

    void reset() {
        _position = glm::vec3( 0.0f );
        _rotation = glm::vec3( 0.0f );
        _scale    = glm::vec3( 1.0f );
        _dirty    = true;
    }

private:
    // --- Acceso interno a la model matrix (usado por RenderEntityHandle) --------------

    friend class RenderEntityHandle;

    // Devuelve la model matrix actualizada. Si el transform no ha sido
    // modificado desde la ultima llamada, devuelve la matrix cacheada.
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
    glm::vec3 _rotation{ 0.0f };   // radianes, orden XYZ
    glm::vec3 _scale   { 1.0f };

    mutable glm::mat4 _modelMatrix{ 1.0f };
    mutable bool      _dirty = true;
};
