#pragma once

#include <unordered_map>
#include <vector>

#include "Entity.h"

class Scene {
public:
    EntityId createEntity() {
        Entity entity{ _nextEntityId++ };

        const std::size_t index = _entities.size();
        _entities.push_back( entity );
        _entityIndexById[entity.getId()] = index;

        return entity.getId();
    }

    EntityId createEntity( MeshHandle mesh, MaterialHandle material, const Entity::Transform& transform ) {
        const EntityId id = createEntity();
        Entity* entity = getEntity( id );
        if (entity != nullptr) {
            entity->setMesh( mesh );
            entity->setMaterial( material );
            entity->setTransform( transform );
        }
        return id;
    }

    bool destroyEntity( EntityId id ) {
        const auto it = _entityIndexById.find( id );
        if (it == _entityIndexById.end()) {
            return false;
        }

        const std::size_t index = it->second;
        const std::size_t lastIndex = _entities.size() - 1;

        if (index != lastIndex) {
            _entities[index] = _entities[lastIndex];
            _entityIndexById[_entities[index].getId()] = index;
        }

        _entities.pop_back();
        _entityIndexById.erase( it );
        return true;
    }

    Entity* getEntity( EntityId id ) {
        const auto it = _entityIndexById.find( id );
        if (it == _entityIndexById.end()) {
            return nullptr;
        }

        return &_entities[it->second];
    }

    const Entity* getEntity( EntityId id ) const {
        const auto it = _entityIndexById.find( id );
        if (it == _entityIndexById.end()) {
            return nullptr;
        }

        return &_entities[it->second];
    }

    bool hasEntity( EntityId id ) const {
        return _entityIndexById.find( id ) != _entityIndexById.end();
    }

    void clear() {
        _entities.clear();
        _entityIndexById.clear();
        _nextEntityId = 1;
    }

    std::size_t count() const {
        return _entities.size();
    }

    const std::vector<Entity>& entities() const {
        return _entities;
    }

    const std::vector<RenderObject>& buildRenderQueue() const {
        _renderQueueCache.clear();
        _renderQueueCache.reserve( _entities.size() );

        for (const Entity& entity : _entities) {
            if (!entity.isActive() || !entity.isVisible() || !entity.hasMesh()) {
                continue;
            }

            _renderQueueCache.push_back( entity.getRenderObjectInternal() );
        }

        return _renderQueueCache;
    }

private:
    std::vector<Entity> _entities;
    std::unordered_map<EntityId, std::size_t> _entityIndexById;
    mutable std::vector<RenderObject> _renderQueueCache;
    EntityId _nextEntityId = 1;
};
