#include "RenderEntityHandle.h"

#include <utility>

namespace {
Scene& requireHandleScene( Scene* scene, uint32_t generation, const char* callSite ) {
    if (scene == nullptr || generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            std::string( callSite ) + ": handle is not initialized"
        );
    }
    return *scene;
}

const Scene& requireHandleSceneConst( const Scene* scene, uint32_t generation, const char* callSite ) {
    if (scene == nullptr || generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            std::string( callSite ) + ": handle is not initialized"
        );
    }
    return *scene;
}
}

RenderEntityHandle::RenderEntityHandle( RenderEntityHandle&& other ) noexcept
    : _index( other._index )
    , _generation( other._generation )
    , _scene( other._scene )
    , _transform( std::move( other._transform ) ) {
    other.invalidate();
}

RenderEntityHandle& RenderEntityHandle::operator=( RenderEntityHandle&& other ) noexcept {
    if (this != &other) {
        _index      = other._index;
        _generation = other._generation;
        _scene      = other._scene;
        _transform  = std::move( other._transform );
        other.invalidate();
    }
    return *this;
}

void RenderEntityHandle::invalidate() {
    _index      = 0;
    _generation = 0;
    _scene      = nullptr;
}

bool RenderEntityHandle::isValid() const {
    if (_scene == nullptr || _generation == 0) {
        return false;
    }
    return _scene->hasEntity( *this );
}

void RenderEntityHandle::flushModelMatrix() {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::flushModelMatrix" )
        .requireSlot( _index, _generation, "RenderEntityHandle::flushModelMatrix" )
        .renderObject.modelMatrix = _transform.getModelMatrix();
}

void RenderEntityHandle::setPosition( const glm::vec3& position ) {
    _transform.setPosition( position );
    flushModelMatrix();
}

void RenderEntityHandle::setRotation( const glm::vec3& rotationRadians ) {
    _transform.setRotation( rotationRadians );
    flushModelMatrix();
}

void RenderEntityHandle::setScale( const glm::vec3& scale ) {
    _transform.setScale( scale );
    flushModelMatrix();
}

void RenderEntityHandle::setScale( float uniformScale ) {
    _transform.setScale( uniformScale );
    flushModelMatrix();
}

void RenderEntityHandle::setTransform( const Transform& transform ) {
    _transform = transform;
    flushModelMatrix();
}

void RenderEntityHandle::setTransform( const glm::vec3& position,
                                       const glm::vec3& rotationRadians,
                                       const glm::vec3& scale ) {
    _transform.setPosition( position );
    _transform.setRotation( rotationRadians );
    _transform.setScale( scale );
    flushModelMatrix();
}

void RenderEntityHandle::translate( const glm::vec3& delta ) {
    _transform.translate( delta );
    flushModelMatrix();
}

void RenderEntityHandle::rotate( const glm::vec3& deltaRadians ) {
    _transform.rotate( deltaRadians );
    flushModelMatrix();
}

void RenderEntityHandle::rotateX( float radians ) {
    _transform.rotateX( radians );
    flushModelMatrix();
}

void RenderEntityHandle::rotateY( float radians ) {
    _transform.rotateY( radians );
    flushModelMatrix();
}

void RenderEntityHandle::rotateZ( float radians ) {
    _transform.rotateZ( radians );
    flushModelMatrix();
}

void RenderEntityHandle::scale( const glm::vec3& factor ) {
    _transform.scale( factor );
    flushModelMatrix();
}

void RenderEntityHandle::scale( float uniformFactor ) {
    _transform.scale( uniformFactor );
    flushModelMatrix();
}

void RenderEntityHandle::resetTransform() {
    _transform.reset();
    flushModelMatrix();
}

void RenderEntityHandle::setMesh( MeshHandle mesh ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setMesh" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setMesh" )
        .renderObject.mesh = mesh;
}

void RenderEntityHandle::setMaterial( MaterialHandle material ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setMaterial" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setMaterial" )
        .renderObject.material = material;
}

void RenderEntityHandle::clearMesh() {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::clearMesh" )
        .requireSlot( _index, _generation, "RenderEntityHandle::clearMesh" )
        .renderObject.mesh = MeshHandle{};
}

void RenderEntityHandle::clearMaterial() {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::clearMaterial" )
        .requireSlot( _index, _generation, "RenderEntityHandle::clearMaterial" )
        .renderObject.material = MaterialHandle{};
}

MeshHandle RenderEntityHandle::getMesh() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::getMesh" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::getMesh" )
        .renderObject.mesh;
}

MaterialHandle RenderEntityHandle::getMaterial() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::getMaterial" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::getMaterial" )
        .renderObject.material;
}

bool RenderEntityHandle::hasMesh() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::hasMesh" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::hasMesh" )
        .renderObject.mesh.isValid();
}

bool RenderEntityHandle::hasMaterial() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::hasMaterial" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::hasMaterial" )
        .renderObject.material.isValid();
}

void RenderEntityHandle::setActive( bool active ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setActive" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setActive" )
        .active = active;
}

void RenderEntityHandle::setVisible( bool visible ) {
    requireHandleScene( _scene, _generation, "RenderEntityHandle::setVisible" )
        .requireSlot( _index, _generation, "RenderEntityHandle::setVisible" )
        .visible = visible;
}

bool RenderEntityHandle::isActive() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::isActive" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::isActive" )
        .active;
}

bool RenderEntityHandle::isVisible() const {
    return requireHandleSceneConst( _scene, _generation, "RenderEntityHandle::isVisible" )
        .requireSlotConst( _index, _generation, "RenderEntityHandle::isVisible" )
        .visible;
}
