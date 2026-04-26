#include "LightEntityHandle.h"

namespace {
Scene& requireLightHandleScene( Scene* scene, uint32_t generation, const char* callSite ) {
    if (scene == nullptr || generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            std::string( callSite ) + ": light handle is not initialized"
        );
    }
    return *scene;
}

const Scene& requireLightHandleSceneConst( const Scene* scene, uint32_t generation, const char* callSite ) {
    if (scene == nullptr || generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            std::string( callSite ) + ": light handle is not initialized"
        );
    }
    return *scene;
}
}

LightEntityHandle::LightEntityHandle( LightEntityHandle&& other ) noexcept
    : _index( other._index )
    , _generation( other._generation )
    , _scene( other._scene ) {
    other.invalidate();
}

LightEntityHandle& LightEntityHandle::operator=( LightEntityHandle&& other ) noexcept {
    if (this != &other) {
        _index      = other._index;
        _generation = other._generation;
        _scene      = other._scene;
        other.invalidate();
    }
    return *this;
}

void LightEntityHandle::invalidate() {
    _index      = 0;
    _generation = 0;
    _scene      = nullptr;
}

bool LightEntityHandle::isValid() const {
    if (_scene == nullptr || _generation == 0) {
        return false;
    }
    return _scene->hasLight( *this );
}

void LightEntityHandle::setPosOrDir( const glm::vec3& posOrDir ) {
    requireLightHandleScene( _scene, _generation, "LightEntityHandle::setPosOrDir" )
        .requireLightSlot( _index, _generation, "LightEntityHandle::setPosOrDir" )
        .light.posOrDir = posOrDir;
}

void LightEntityHandle::setColor( const glm::vec3& color ) {
    requireLightHandleScene( _scene, _generation, "LightEntityHandle::setColor" )
        .requireLightSlot( _index, _generation, "LightEntityHandle::setColor" )
        .light.color = color;
}

void LightEntityHandle::setIntensity( float intensity ) {
    requireLightHandleScene( _scene, _generation, "LightEntityHandle::setIntensity" )
        .requireLightSlot( _index, _generation, "LightEntityHandle::setIntensity" )
        .light.intensity = intensity;
}

void LightEntityHandle::setRange( float range ) {
    requireLightHandleScene( _scene, _generation, "LightEntityHandle::setRange" )
        .requireLightSlot( _index, _generation, "LightEntityHandle::setRange" )
        .light.range = range;
}

void LightEntityHandle::setType( LightType type ) {
    requireLightHandleScene( _scene, _generation, "LightEntityHandle::setType" )
        .requireLightSlot( _index, _generation, "LightEntityHandle::setType" )
        .light.type = type;
}

void LightEntityHandle::setActive( bool active ) {
    requireLightHandleScene( _scene, _generation, "LightEntityHandle::setActive" )
        .requireLightSlot( _index, _generation, "LightEntityHandle::setActive" )
        .active = active;
}

glm::vec3 LightEntityHandle::getPosOrDir() const {
    return requireLightHandleSceneConst( _scene, _generation, "LightEntityHandle::getPosOrDir" )
        .requireLightSlotConst( _index, _generation, "LightEntityHandle::getPosOrDir" )
        .light.posOrDir;
}

glm::vec3 LightEntityHandle::getColor() const {
    return requireLightHandleSceneConst( _scene, _generation, "LightEntityHandle::getColor" )
        .requireLightSlotConst( _index, _generation, "LightEntityHandle::getColor" )
        .light.color;
}

float LightEntityHandle::getIntensity() const {
    return requireLightHandleSceneConst( _scene, _generation, "LightEntityHandle::getIntensity" )
        .requireLightSlotConst( _index, _generation, "LightEntityHandle::getIntensity" )
        .light.intensity;
}

float LightEntityHandle::getRange() const {
    return requireLightHandleSceneConst( _scene, _generation, "LightEntityHandle::getRange" )
        .requireLightSlotConst( _index, _generation, "LightEntityHandle::getRange" )
        .light.range;
}

LightType LightEntityHandle::getType() const {
    return requireLightHandleSceneConst( _scene, _generation, "LightEntityHandle::getType" )
        .requireLightSlotConst( _index, _generation, "LightEntityHandle::getType" )
        .light.type;
}

bool LightEntityHandle::isActive() const {
    return requireLightHandleSceneConst( _scene, _generation, "LightEntityHandle::isActive" )
        .requireLightSlotConst( _index, _generation, "LightEntityHandle::isActive" )
        .active;
}
