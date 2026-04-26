#include "MaterialHandle.h"

#include <string>

#include "ResourceManager.h"

namespace {
ResourceManager& requireMaterialHandleManager( ResourceManager* manager, const char* callSite ) {
    if (manager == nullptr) {
        throw ResourceException(
            ResourceErrorCode::InvalidHandle,
            std::string( callSite ) + ": material handle is not initialized"
        );
    }
    return *manager;
}

const ResourceManager& requireMaterialHandleManagerConst( const ResourceManager* manager, const char* callSite ) {
    if (manager == nullptr) {
        throw ResourceException(
            ResourceErrorCode::InvalidHandle,
            std::string( callSite ) + ": material handle is not initialized"
        );
    }
    return *manager;
}
}

bool MaterialHandle::isValid() const {
    if (_manager == nullptr || _generation == INVALID_HANDLE_GENERATION || _index == INVALID_HANDLE_INDEX) {
        return false;
    }
    return _manager->tryGetMaterialSlot( *this ) != nullptr;
}

void MaterialHandle::setBaseColor( const glm::vec4& baseColor ) {
    requireMaterialHandleManager( _manager, "MaterialHandle::setBaseColor" )
        .requireMaterialSlot( *this, "MaterialHandle::setBaseColor" )
        .data.baseColor = baseColor;
}

void MaterialHandle::setMetallic( float metallic ) {
    requireMaterialHandleManager( _manager, "MaterialHandle::setMetallic" )
        .requireMaterialSlot( *this, "MaterialHandle::setMetallic" )
        .data.metallic = metallic;
}

void MaterialHandle::setRoughness( float roughness ) {
    requireMaterialHandleManager( _manager, "MaterialHandle::setRoughness" )
        .requireMaterialSlot( *this, "MaterialHandle::setRoughness" )
        .data.roughness = roughness;
}

void MaterialHandle::setBaseColorTexture( TextureHandle texture ) {
    ResourceManager& manager = requireMaterialHandleManager( _manager, "MaterialHandle::setBaseColorTexture" );
    ResourceManager::MaterialSlot& slot = manager.requireMaterialSlot( *this, "MaterialHandle::setBaseColorTexture" );
    manager.validateMaterialTextureHandle( texture, "MaterialHandle::setBaseColorTexture" );
    slot.baseColorTexture = texture;
    slot.data.textureIndex = texture.isValid() ? static_cast<int>( texture.index ) : -1;
}

void MaterialHandle::setNormalTexture( TextureHandle texture ) {
    ResourceManager& manager = requireMaterialHandleManager( _manager, "MaterialHandle::setNormalTexture" );
    ResourceManager::MaterialSlot& slot = manager.requireMaterialSlot( *this, "MaterialHandle::setNormalTexture" );
    manager.validateMaterialTextureHandle( texture, "MaterialHandle::setNormalTexture" );
    slot.normalTexture = texture;
    slot.data.normalTextureIndex = texture.isValid() ? static_cast<int>( texture.index ) : -1;
}

glm::vec4 MaterialHandle::getBaseColor() const {
    return requireMaterialHandleManagerConst( _manager, "MaterialHandle::getBaseColor" )
        .requireMaterialSlotConst( *this, "MaterialHandle::getBaseColor" )
        .data.baseColor;
}

float MaterialHandle::getMetallic() const {
    return requireMaterialHandleManagerConst( _manager, "MaterialHandle::getMetallic" )
        .requireMaterialSlotConst( *this, "MaterialHandle::getMetallic" )
        .data.metallic;
}

float MaterialHandle::getRoughness() const {
    return requireMaterialHandleManagerConst( _manager, "MaterialHandle::getRoughness" )
        .requireMaterialSlotConst( *this, "MaterialHandle::getRoughness" )
        .data.roughness;
}

TextureHandle MaterialHandle::getBaseColorTexture() const {
    return requireMaterialHandleManagerConst( _manager, "MaterialHandle::getBaseColorTexture" )
        .requireMaterialSlotConst( *this, "MaterialHandle::getBaseColorTexture" )
        .baseColorTexture;
}

TextureHandle MaterialHandle::getNormalTexture() const {
    return requireMaterialHandleManagerConst( _manager, "MaterialHandle::getNormalTexture" )
        .requireMaterialSlotConst( *this, "MaterialHandle::getNormalTexture" )
        .normalTexture;
}

void MaterialHandle::clearBaseColorTexture() {
    ResourceManager::MaterialSlot& slot = requireMaterialHandleManager( _manager, "MaterialHandle::clearBaseColorTexture" )
        .requireMaterialSlot( *this, "MaterialHandle::clearBaseColorTexture" );
    slot.baseColorTexture = TextureHandle{};
    slot.data.textureIndex = -1;
}

void MaterialHandle::clearNormalTexture() {
    ResourceManager::MaterialSlot& slot = requireMaterialHandleManager( _manager, "MaterialHandle::clearNormalTexture" )
        .requireMaterialSlot( *this, "MaterialHandle::clearNormalTexture" );
    slot.normalTexture = TextureHandle{};
    slot.data.normalTextureIndex = -1;
}
