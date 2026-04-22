#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "BufferObjectsData.h"

using EntityId = std::uint32_t;

constexpr EntityId INVALID_ENTITY_ID = 0;

class Entity {
public:
    struct Transform {
    public:
        Transform() = default;

        Transform( const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale )
            : _position( position ), _rotation( rotation ), _scale( scale ) {
        }

        const glm::vec3& getPosition() const {
            return _position;
        }

        const glm::vec3& getRotation() const {
            return _rotation;
        }

        const glm::vec3& getScale() const {
            return _scale;
        }

        void setPosition( const glm::vec3& position ) {
            _position = position;
        }

        void setRotation( const glm::vec3& rotationRadians ) {
            _rotation = rotationRadians;
        }

        void setScale( const glm::vec3& scale ) {
            _scale = scale;
        }

    private:
        glm::vec3 _position{ 0.0f };
        glm::vec3 _rotation{ 0.0f };
        glm::vec3 _scale{ 1.0f };
    };

    Entity() = default;

    explicit Entity( EntityId id )
        : _id( id ) {
    }

    EntityId getId() const {
        return _id;
    }

    bool isActive() const {
        return _active;
    }

    void setActive( bool active ) {
        _active = active;
    }

    bool isVisible() const {
        return _visible;
    }

    void setVisible( bool visible ) {
        _visible = visible;
    }

    void setMesh( MeshHandle mesh ) {
        _renderObject.mesh = mesh;
    }

    MeshHandle getMesh() const {
        return _renderObject.mesh;
    }

    bool hasMesh() const {
        return _renderObject.mesh.isValid();
    }

    void clearMesh() {
        _renderObject.mesh = MeshHandle{};
    }

    void setMaterial( MaterialHandle material ) {
        _renderObject.material = material;
    }

    MaterialHandle getMaterial() const {
        return _renderObject.material;
    }

    bool hasMaterial() const {
        return _renderObject.material.isValid();
    }

    void clearMaterial() {
        _renderObject.material = MaterialHandle{};
    }

    void setModelMatrix( const glm::mat4& modelMatrix ) {
        _renderObject.modelMatrix = modelMatrix;
    }

    const glm::mat4& getModelMatrix() const {
        return _renderObject.modelMatrix;
    }

    const Transform& getTransform() const {
        return _transform;
    }

    void setTransform( const glm::vec3& position, const glm::vec3& rotationRadians, const glm::vec3& scale = glm::vec3( 1.0f ) ) {
        _transform.setPosition( position );
        _transform.setRotation( rotationRadians );
        _transform.setScale( scale );
        rebuildModelMatrixFromTransform();
    }

    void setTransform( const Transform& transform ) {
        _transform = transform;
        rebuildModelMatrixFromTransform();
    }

private:
    friend class Scene;

    const RenderObject& getRenderObjectInternal() const {
        return _renderObject;
    }

private:
    void rebuildModelMatrixFromTransform() {
        glm::mat4 modelMatrix{ 1.0f };
        modelMatrix = glm::translate( modelMatrix, _transform.getPosition() );
        modelMatrix = glm::rotate( modelMatrix, _transform.getRotation().x, glm::vec3( 1.0f, 0.0f, 0.0f ) );
        modelMatrix = glm::rotate( modelMatrix, _transform.getRotation().y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
        modelMatrix = glm::rotate( modelMatrix, _transform.getRotation().z, glm::vec3( 0.0f, 0.0f, 1.0f ) );
        modelMatrix = glm::scale( modelMatrix, _transform.getScale() );
        _renderObject.modelMatrix = modelMatrix;
    }

private:
    EntityId _id = INVALID_ENTITY_ID;
    Transform _transform;
    RenderObject _renderObject{};
    bool _active = true;
    bool _visible = true;
};
