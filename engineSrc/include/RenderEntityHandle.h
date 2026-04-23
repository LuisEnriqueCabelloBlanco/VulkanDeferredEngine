#pragma once

#include <cstdint>
#include <string>

#include "Scene.h"
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

    RenderEntityHandle( RenderEntityHandle&& other ) noexcept
        : _index( other._index )
        , _generation( other._generation )
        , _scene( other._scene )
        , _transform( std::move( other._transform ) ) {
        other.invalidate();
    }

    RenderEntityHandle& operator=( RenderEntityHandle&& other ) noexcept {
        if (this != &other) {
            _index      = other._index;
            _generation = other._generation;
            _scene      = other._scene;
            _transform  = std::move( other._transform );
            other.invalidate();
        }
        return *this;
    }

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

    // --- Acceso a los ids internos (uso por Scene y RenderEngine) -----------

    uint32_t getIndex()      const { return _index; }
    uint32_t getGeneration() const { return _generation; }

private:
    friend class Scene;

    RenderEntityHandle( uint32_t index, uint32_t generation, Scene* scene )
        : _index( index ), _generation( generation ), _scene( scene ) {
    }

    void invalidate() {
        _index      = 0;
        _generation = 0;
        _scene      = nullptr;
    }

    // Escribe la model matrix actual del transform en el slot de la Scene.
    void flushModelMatrix();

private:
    uint32_t  _index      = 0;
    uint32_t  _generation = 0;   // 0 == handle nunca inicializado
    Scene*    _scene      = nullptr;
    Transform _transform;
};

// ---------------------------------------------------------------------------
// Helper interno de validacion del handle
// ---------------------------------------------------------------------------

inline Scene& requireHandleScene( Scene* scene, uint32_t generation, const char* callSite ) {
    if (scene == nullptr || generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            std::string( callSite ) + ": handle is not initialized"
        );
    }
    return *scene;
}

inline const Scene& requireHandleSceneConst( const Scene* scene, uint32_t generation, const char* callSite ) {
    if (scene == nullptr || generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            std::string( callSite ) + ": handle is not initialized"
        );
    }
    return *scene;
}

// ---------------------------------------------------------------------------
// Implementacion inline
// (vive en el header porque depende de la definicion completa de Scene)
// ---------------------------------------------------------------------------

inline bool RenderEntityHandle::isValid() const {
    if (_scene == nullptr || _generation == 0) {
        return false;
    }
    return _scene->hasEntity( *this );
}

inline void RenderEntityHandle::flushModelMatrix() {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::flushModelMatrix" )
        .requireSlot( _index, _generation, "RenderEntityHandle::flushModelMatrix" )
        .renderObject.modelMatrix = _transform.getModelMatrix();
}

// --- Transform: setters -----------------------------------------------------

inline void RenderEntityHandle::setPosition( const glm::vec3& position ) {
    _transform.setPosition( position );
    flushModelMatrix();
}

inline void RenderEntityHandle::setRotation( const glm::vec3& rotationRadians ) {
    _transform.setRotation( rotationRadians );
    flushModelMatrix();
}

inline void RenderEntityHandle::setScale( const glm::vec3& scale ) {
    _transform.setScale( scale );
    flushModelMatrix();
}

inline void RenderEntityHandle::setScale( float uniformScale ) {
    _transform.setScale( uniformScale );
    flushModelMatrix();
}

inline void RenderEntityHandle::setTransform( const Transform& transform ) {
    _transform = transform;
    flushModelMatrix();
}

inline void RenderEntityHandle::setTransform( const glm::vec3& position,
                                              const glm::vec3& rotationRadians,
                                              const glm::vec3& scale ) {
    _transform.setPosition( position );
    _transform.setRotation( rotationRadians );
    _transform.setScale( scale );
    flushModelMatrix();
}

// --- Transform: mutaciones relativas ----------------------------------------

inline void RenderEntityHandle::translate( const glm::vec3& delta ) {
    _transform.translate( delta );
    flushModelMatrix();
}

inline void RenderEntityHandle::rotate( const glm::vec3& deltaRadians ) {
    _transform.rotate( deltaRadians );
    flushModelMatrix();
}

inline void RenderEntityHandle::rotateX( float radians ) {
    _transform.rotateX( radians );
    flushModelMatrix();
}

inline void RenderEntityHandle::rotateY( float radians ) {
    _transform.rotateY( radians );
    flushModelMatrix();
}

inline void RenderEntityHandle::rotateZ( float radians ) {
    _transform.rotateZ( radians );
    flushModelMatrix();
}

inline void RenderEntityHandle::scale( const glm::vec3& factor ) {
    _transform.scale( factor );
    flushModelMatrix();
}

inline void RenderEntityHandle::scale( float uniformFactor ) {
    _transform.scale( uniformFactor );
    flushModelMatrix();
}

inline void RenderEntityHandle::resetTransform() {
    _transform.reset();
    flushModelMatrix();
}

// --- Mesh y material --------------------------------------------------------

inline void RenderEntityHandle::setMesh( MeshHandle mesh ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setMesh" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setMesh" )
        .renderObject.mesh = mesh;
}

inline void RenderEntityHandle::setMaterial( MaterialHandle material ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setMaterial" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setMaterial" )
        .renderObject.material = material;
}

inline void RenderEntityHandle::clearMesh() {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::clearMesh" )
        .requireSlot( _index, _generation, "RenderEntityHandle::clearMesh" )
        .renderObject.mesh = MeshHandle{};
}

inline void RenderEntityHandle::clearMaterial() {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::clearMaterial" )
        .requireSlot( _index, _generation, "RenderEntityHandle::clearMaterial" )
        .renderObject.material = MaterialHandle{};
}

inline MeshHandle RenderEntityHandle::getMesh() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::getMesh" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::getMesh" )
        .renderObject.mesh;
}

inline MaterialHandle RenderEntityHandle::getMaterial() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::getMaterial" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::getMaterial" )
        .renderObject.material;
}

inline bool RenderEntityHandle::hasMesh() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::hasMesh" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::hasMesh" )
        .renderObject.mesh.isValid();
}

inline bool RenderEntityHandle::hasMaterial() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::hasMaterial" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::hasMaterial" )
        .renderObject.material.isValid();
}

// --- Flags de estado --------------------------------------------------------

inline void RenderEntityHandle::setActive( bool active ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setActive" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setActive" )
        .active = active;
}

inline void RenderEntityHandle::setVisible( bool visible ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setVisible" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setVisible" )
        .visible = visible;
}

inline bool RenderEntityHandle::isActive() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::isActive" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::isActive" )
        .active;
}

inline bool RenderEntityHandle::isVisible() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::isVisible" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::isVisible" )
        .visible;
}
