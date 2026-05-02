#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "CameraHandle.h"
#include "ResourceManager.h"
#include "RenderEngine.h"
#include "RenderEntityHandle.h"
#include "LightEntityHandle.h"
#include "Scene.h"
#include "WindowEvent.h"

class EngineAPI {
public:
    // Barrera publica del motor.
    // La fachada no unifica excepciones de dominio en esta iteracion:
    // pueden propagarse SceneException y ResourceException hacia la app.
    EngineAPI()
        : _engine(), _scene( nullptr ), _resources( nullptr ), _initialized( false ) {
    }

    void init( const std::string& appName ) {
        if (_initialized) {
            throw std::logic_error( "EngineAPI::init called twice" );
        }
        _engine.init( appName );
        _scene = &_engine.getScene();
        _resources = &_engine.getResourceManager();
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

    Scene& getScene() {
        requireInitialized( "EngineAPI::getScene" );
        return *_scene;
    }

    ResourceManager& getResourceManager() {
        requireInitialized( "EngineAPI::getResourceManager" );
        return *_resources;
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
