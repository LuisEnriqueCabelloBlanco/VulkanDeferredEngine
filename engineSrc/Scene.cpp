#include "Scene.h"
#include "RenderEntityHandle.h"

// ---------------------------------------------------------------------------
// Scene — ciclo de vida de entidades
// ---------------------------------------------------------------------------

RenderEntityHandle Scene::createEntity() {
    const uint32_t index = allocateSlot();
    EntitySlot& slot = _slots[index];
    slot.occupied     = true;
    slot.active       = true;
    slot.visible      = true;
    slot.renderObject = RenderObject{};

    return RenderEntityHandle{ index, getSlotGeneration( index ), this };
}

RenderEntityHandle Scene::createEntity( MeshHandle mesh,
                                         MaterialHandle material,
                                         const Transform& transform ) {
    RenderEntityHandle handle = createEntity();
    handle.setMesh( mesh );
    handle.setMaterial( material );
    handle.setTransform( transform );
    return handle;
}

void Scene::destroyEntity( RenderEntityHandle& handle ) {
    if (handle._scene == nullptr || handle._generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            "Scene::destroyEntity received a handle that was never initialized"
        );
    }

    if (handle._scene != this) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            "Scene::destroyEntity received a handle from a different Scene"
        );
    }

    EntitySlot& slot = requireSlot( handle._index, handle._generation, "Scene::destroyEntity" );
    slot.renderObject = RenderObject{};
    slot.occupied     = false;
    bumpGeneration( slot.generation );
    _freeSlots.push_back( handle._index );

    handle.invalidate();
}

void Scene::clear() {
    for (EntitySlot& slot : _slots) {
        if (slot.occupied) {
            bumpGeneration( slot.generation );
        }
        slot.renderObject = RenderObject{};
        slot.occupied     = false;
        slot.active       = true;
        slot.visible      = true;
    }

    _freeSlots.clear();
    _freeSlots.reserve( _slots.size() );
    for (uint32_t i = 0; i < _slots.size(); ++i) {
        _freeSlots.push_back( i );
    }
}

// ---------------------------------------------------------------------------
// Scene — consultas generales
// ---------------------------------------------------------------------------

bool Scene::hasEntity( const RenderEntityHandle& handle ) const {
    if (handle._scene != this || handle._generation == 0) {
        return false;
    }
    return hasEntity( handle._index, handle._generation );
}

std::size_t Scene::entityCount() const {
    return _slots.size() - _freeSlots.size();
}

// ---------------------------------------------------------------------------
// Scene — acceso interno a slots (usado por RenderEntityHandle)
// ---------------------------------------------------------------------------

Scene::EntitySlot& Scene::requireSlot( uint32_t index, uint32_t generation, const char* callSite ) {
    EntitySlot* slot = tryGetSlot( index, generation );

    if (slot == nullptr) {
        if (!isSlotIndexInRange( index )) {
            throw SceneException(
                SceneErrorCode::InvalidHandle,
                std::string( callSite ) + ": handle index is out of range"
            );
        }
        throw SceneException(
            SceneErrorCode::StaleHandle,
            std::string( callSite ) + ": entity has already been destroyed"
        );
    }

    return *slot;
}

const Scene::EntitySlot& Scene::requireSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const {
    const EntitySlot* slot = tryGetSlot( index, generation );

    if (slot == nullptr) {
        if (!isSlotIndexInRange( index )) {
            throw SceneException(
                SceneErrorCode::InvalidHandle,
                std::string( callSite ) + ": handle index is out of range"
            );
        }
        throw SceneException(
            SceneErrorCode::StaleHandle,
            std::string( callSite ) + ": entity has already been destroyed"
        );
    }

    return *slot;
}

// ---------------------------------------------------------------------------
// Scene — render queue (exclusivo para RenderEngine)
// ---------------------------------------------------------------------------

const std::vector<RenderObject>& Scene::buildRenderQueue() const {
    _renderQueueCache.clear();
    _renderQueueCache.reserve( entityCount() );

    for (const EntitySlot& slot : _slots) {
        if (!slot.occupied || !slot.active || !slot.visible) {
            continue;
        }
        if (!slot.renderObject.mesh.isValid()) {
            continue;
        }
        _renderQueueCache.push_back( slot.renderObject );
    }

    return _renderQueueCache;
}

// ---------------------------------------------------------------------------
// Scene — implementacion interna
// ---------------------------------------------------------------------------

bool Scene::hasEntity( uint32_t index, uint32_t generation ) const {
    return tryGetSlot( index, generation ) != nullptr;
}

Scene::EntitySlot* Scene::tryGetSlot( uint32_t index, uint32_t generation ) {
    if (index >= _slots.size()) {
        return nullptr;
    }
    EntitySlot& slot = _slots[index];
    if (!slot.occupied || slot.generation != generation) {
        return nullptr;
    }
    return &slot;
}

const Scene::EntitySlot* Scene::tryGetSlot( uint32_t index, uint32_t generation ) const {
    if (index >= _slots.size()) {
        return nullptr;
    }
    const EntitySlot& slot = _slots[index];
    if (!slot.occupied || slot.generation != generation) {
        return nullptr;
    }
    return &slot;
}

bool Scene::isSlotIndexInRange( uint32_t index ) const {
    return index < _slots.size();
}

uint32_t Scene::getSlotGeneration( uint32_t index ) const {
    return _slots[index].generation;
}

uint32_t Scene::allocateSlot() {
    if (!_freeSlots.empty()) {
        const uint32_t index = _freeSlots.back();
        _freeSlots.pop_back();
        return index;
    }

    if (_slots.size() >= MAX_ENTITIES) {
        throw SceneException(
            SceneErrorCode::LimitExceeded,
            "Scene::createEntity exceeded MAX_ENTITIES (" + std::to_string( MAX_ENTITIES ) + ")"
        );
    }

    _slots.emplace_back();
    return static_cast<uint32_t>(_slots.size() - 1);
}

void Scene::bumpGeneration( uint32_t& generation ) {
    ++generation;
    if (generation == 0) {
        generation = 1;
    }
}
