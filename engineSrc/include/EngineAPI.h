#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "BufferObjectsData.h"
#include "Camera.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "RenderEngine.h"
#include "RenderEntityHandle.h"
#include "Scene.h"
#include "WindowEvent.h"

class EngineAPI {
public:
    EngineAPI()
        : _engine(), _scene( nullptr ), _resources( nullptr ), _initialized( false ) {
    }

    void init( const std::string& appName ) {
        if (_initialized) {
            throw std::logic_error( "EngineAPI::init called twice" );
        }
        _engine.init( appName );
        _scene = &_engine.getSceneInternal();
        _resources = &_engine.getResourceManagerInternal();
        _initialized = true;
    }

    void cleanup() {
        requireInitialized( "EngineAPI::cleanup" );
        _engine.cleanup();
        _scene = nullptr;
        _resources = nullptr;
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

    ResourceManager& getResourceManager() {
        requireInitialized( "EngineAPI::getResourceManager" );
        return *_resources;
    }

    const ResourceManager& getResourceManager() const {
        requireInitialized( "EngineAPI::getResourceManager const" );
        return *_resources;
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
    ResourceManager* _resources;
    bool          _initialized;
};
