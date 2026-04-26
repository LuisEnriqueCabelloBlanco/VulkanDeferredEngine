#pragma once

#include <cstdint>
#include <string>

#include "Scene.h"
#include "MaterialHandle.h"
#include "ResourceHandles.h"
#include "Transform.h"

// ---------------------------------------------------------------------------
// RenderEntityHandle
// ---------------------------------------------------------------------------

/*
RenderEntityHandle es el unico token que la aplicacion recibe al crear una entidad.

Encapsula el par (index, generation) que identifica un slot en la Scene,
mas un puntero a la Scene que lo emitio. Toda mutacion y consulta se despacha
a traves del handle, sin que el caller necesite acceder a Scene directamente.

El handle es move-only: un handle, una entidad, un objeto renderizable.
Copiar handles queda prohibido para evitar comportamientos indeterminados.
El handle almacena un Transform local. Cualquier mutacion sobre el transform
actualiza automaticamente la model matrix del slot correspondiente en la Scene.

Uso tipico:
    RenderEntityHandle entity = scene.createEntity(
        mesh,
        material,
        Transform{
            { 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f }
        }
    );
    entity.translate( { 0.0f, 1.0f, 0.0f } );
    entity.setVisible( false );
    // Al salir de scope NO destruye la entidad; el caller decide cuando llamar destroyEntity().
*/
class RenderEntityHandle {
public:
    // --- Construccion y ciclo de vida del handle ----------------------------

    RenderEntityHandle() = default;

    RenderEntityHandle( const RenderEntityHandle& ) = delete;
    RenderEntityHandle& operator=( const RenderEntityHandle& ) = delete;

    RenderEntityHandle( RenderEntityHandle&& other ) noexcept;
    RenderEntityHandle& operator=( RenderEntityHandle&& other ) noexcept;

    ~RenderEntityHandle() = default;

    // Devuelve true si el handle fue emitido por su Scene y la entidad sigue viva.
    // No lanza excepciones.
    bool isValid() const;

    // --- Transform: setters (override) --------------------------------------

    // Contrato de errores para TODOS los metodos publicos de transform:
    // - SceneException(InvalidHandle): handle no inicializado.
    // - SceneException(StaleHandle): entidad destruida o invalidada.

    void setPosition( const glm::vec3& position );
    void setRotation( const glm::vec3& rotationRadians );
    void setScale   ( const glm::vec3& scale );
    void setScale   ( float uniformScale );
    void setTransform( const Transform& transform );
    void setTransform( const glm::vec3& position,
                       const glm::vec3& rotationRadians,
                       const glm::vec3& scale = glm::vec3( 1.0f ) );

    // --- Transform: mutaciones relativas ------------------------------------

    void translate( const glm::vec3& delta );
    void rotate   ( const glm::vec3& deltaRadians );
    void rotateX  ( float radians );
    void rotateY  ( float radians );
    void rotateZ  ( float radians );
    void scale    ( const glm::vec3& factor );
    void scale    ( float uniformFactor );

    // Resetea position a (0,0,0), rotation a (0,0,0) y scale a (1,1,1).
    void resetTransform();

    // --- Transform: getters -------------------------------------------------

    const glm::vec3& getPosition()    const { return _transform.getPosition(); }
    const glm::vec3& getRotation()    const { return _transform.getRotation(); }
    const glm::vec3& getScale()       const { return _transform.getScale(); }

    // --- Mesh y material ----------------------------------------------------

    // Contrato de errores para TODOS los metodos publicos de mesh/material:
    // - SceneException(InvalidHandle): handle no inicializado.
    // - SceneException(StaleHandle): entidad destruida o invalidada.

    void setMesh    ( MeshHandle mesh );
    void setMaterial( MaterialHandle material );
    void clearMesh();
    void clearMaterial();

    MeshHandle     getMesh()     const;
    MaterialHandle getMaterial() const;
    bool           hasMesh()     const;
    bool           hasMaterial() const;

    // --- Flags de estado ----------------------------------------------------

    // Contrato de errores para TODOS los metodos publicos de flags:
    // - SceneException(InvalidHandle): handle no inicializado.
    // - SceneException(StaleHandle): entidad destruida o invalidada.

    void setActive ( bool active  );
    void setVisible( bool visible );
    bool isActive()  const;
    bool isVisible() const;

private:
    friend class Scene;

    RenderEntityHandle( uint32_t index, uint32_t generation, Scene* scene )
        : _index( index ), _generation( generation ), _scene( scene ) {
    }

    void invalidate();

    // Escribe la model matrix actual del transform en el slot de la Scene.
    void flushModelMatrix();

private:
    uint32_t  _index      = 0;
    uint32_t  _generation = 0;   // 0 == handle nunca inicializado
    Scene*    _scene      = nullptr;
    Transform _transform;
};
