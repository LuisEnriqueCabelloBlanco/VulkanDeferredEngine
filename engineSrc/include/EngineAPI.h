#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "BufferObjectsData.h"
#include "Camera.h"
#include "Mesh.h"
#include "RenderEngine.h"
#include "RenderEntityHandle.h"
#include "Scene.h"
#include "WindowEvent.h"

class EngineAPI {
public:
    EngineAPI()
        : _engine(), _scene( nullptr ), _initialized( false ) {
    }

    void init( const std::string& appName ) {
        if (_initialized) {
            throw std::logic_error( "EngineAPI::init called twice" );
        }
        _engine.init( appName );
        _scene = &_engine.getSceneInternal();
        _initialized = true;
    }

    void cleanup() {
        requireInitialized( "EngineAPI::cleanup" );
        _engine.cleanup();
        _scene = nullptr;
        _initialized = false;
    }

    void wait() {
        requireInitialized( "EngineAPI::wait" );
        _engine.wait();
    }

    void drawFrame() {
        requireInitialized( "EngineAPI::drawFrame" );
        _engine.drawFrame();
    }

    void handleWindowEvent( const WindowEvent& event ) {
        requireInitialized( "EngineAPI::handleWindowEvent" );
        _engine.handleWindowEvent( event );
    }

    Camera& getMainCamera() {
        requireInitialized( "EngineAPI::getMainCamera" );
        return _engine.getMainCamera();
    }

    Scene& getScene() {
        requireInitialized( "EngineAPI::getScene" );
        return *_scene;
    }

    MeshHandle createMesh( const std::string& name, const std::string& path ) {
        requireInitialized( "EngineAPI::createMesh(path)" );
        return _engine.createMesh( name, path );
    }

    MeshHandle createMesh( const std::string& name, const std::vector<Vertex>& vertices ) {
        requireInitialized( "EngineAPI::createMesh(vertices)" );
        return _engine.createMesh( name, vertices );
    }

    MeshHandle createMesh( const std::string& name, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices ) {
        requireInitialized( "EngineAPI::createMesh(indices,vertices)" );
        return _engine.createMesh( name, indices, vertices );
    }

    TextureHandle createTexture( const std::string& name, const std::string& path ) {
        requireInitialized( "EngineAPI::createTexture" );
        return _engine.createTexture( name, path );
    }

    MaterialHandle createMaterial( const std::string& name, const MaterialDesc& material ) {
        requireInitialized( "EngineAPI::createMaterial" );
        return _engine.createMaterial( name, material );
    }

    void releaseAllMeshes() {
        requireInitialized( "EngineAPI::releaseAllMeshes" );
        _engine.releaseAllMeshes();
    }

    void releaseAllTextures() {
        requireInitialized( "EngineAPI::releaseAllTextures" );
        _engine.releaseAllTextures();
    }

    void releaseAllMaterials() {
        requireInitialized( "EngineAPI::releaseAllMaterials" );
        _engine.releaseAllMaterials();
    }

    void createPointLight( glm::vec3 position, glm::vec3 color, float intensity, float range, bool preload = false ) {
        requireInitialized( "EngineAPI::createPointLight" );
        _engine.createPointLight( position, color, intensity, range, preload );
    }

    void createDirectionalLight( glm::vec3 direction, glm::vec3 color, float intensity, bool preload = false ) {
        requireInitialized( "EngineAPI::createDirectionalLight" );
        _engine.createDirectionalLight( direction, color, intensity, preload );
    }

    void setMainLight( int index ) {
        requireInitialized( "EngineAPI::setMainLight" );
        _engine.setMainLight( index );
    }

    void updateLightBuffer() {
        requireInitialized( "EngineAPI::updateLightBuffer" );
        _engine.updateLightBuffer();
    }

private:
    void requireInitialized( const char* callSite ) const {
        if (!_initialized || _scene == nullptr) {
            throw std::logic_error( std::string( callSite ) + ": engine is not initialized" );
        }
    }

private:
    RenderEngine  _engine;
    Scene*        _scene;
    bool          _initialized;
};
