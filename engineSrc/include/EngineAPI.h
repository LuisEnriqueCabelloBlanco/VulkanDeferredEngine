#pragma once

/**
 * @file EngineAPI.h
 * @brief Punto de entrada unico para la aplicacion que consume el motor de renderizado.
 *
 * La aplicacion solo necesita incluir este header. El resto de headers del motor
 * son accesibles de forma transitiva a traves de este fichero.
 *
 * Flujo de uso tipico:
 * @code
 * EngineAPI engine;
 * engine.init( "Mi aplicacion" );
 *
 * ResourceManager& res   = engine.getResourceManager();
 * Scene&           scene = engine.getScene();
 *
 * MeshHandle     mesh = res.createMesh    ( "cubo",     "cube.obj" );
 * TextureHandle  tex  = res.createTexture ( "albedo",   "albedo.png" );
 * MaterialHandle mat  = res.createMaterial( "material", { .baseColorTexture = tex } );
 *
 * RenderEntityHandle entity = scene.createEntity( mesh, mat, Transform{} );
 * scene.getCamera().setPosition( { 0.f, 2.f, -5.f } );
 *
 * while ( running ) {
 *     engine.drawFrame();
 * }
 *
 * engine.cleanup();
 * @endcode
 */

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

/**
 * @brief Fachada publica del motor de renderizado.
 *
 * EngineAPI es la unica clase que la aplicacion necesita instanciar. Proporciona
 * acceso a todos los subsistemas del motor (Scene, ResourceManager) y encapsula
 * el ciclo de vida completo: inicializacion, bucle de renderizado y limpieza.
 *
 * La fachada no captura ni unifica excepciones de dominio en esta iteracion:
 * SceneException y ResourceException pueden propagarse hacia la aplicacion.
 * Si se llama a cualquier metodo antes de init() o despues de cleanup(), se
 * lanza std::logic_error.
 *
 * @note La clase no es copiable ni movible. Debe usarse como objeto de vida
 *       larga, tipicamente en el scope principal de la aplicacion.
 */
class EngineAPI {
public:
    /**
     * @brief Construye la fachada en estado no inicializado.
     *
     * Ninguna operacion del motor esta disponible hasta que se llame a init().
     */
    EngineAPI()
        : _engine(), _scene( nullptr ), _resources( nullptr ), _initialized( false ) {
    }

    /**
     * @brief Inicializa el motor y todos sus subsistemas.
     *
     * Crea la ventana, el contexto Vulkan, el swapchain y los recursos internos
     * necesarios para renderizar. Debe llamarse exactamente una vez antes de
     * cualquier otra operacion.
     *
     * @param appName Nombre de la aplicacion, usado en la informacion de instancia Vulkan.
     * @throws std::logic_error si init() ya fue llamado previamente.
     */
    void init( const std::string& appName ) {
        if (_initialized) {
            throw std::logic_error( "EngineAPI::init called twice" );
        }
        _engine.init( appName );
        _scene     = &_engine.getScene();
        _resources = &_engine.getResourceManager();
        _initialized = true;
    }

    /**
     * @brief Libera todos los recursos del motor y lo deja en estado no inicializado.
     *
     * Destruye los recursos GPU, el swapchain, la ventana y cualquier estado interno.
     * Tras cleanup(), el objeto puede descartarse pero no reinicializarse.
     *
     * @throws std::logic_error si el motor no esta inicializado.
     */
    void cleanup() {
        requireInitialized( "EngineAPI::cleanup" );
        _engine.cleanup();
        _scene       = nullptr;
        _resources   = nullptr;
        _initialized = false;
    }

    /**
     * @brief Espera a que todos los comandos pendientes en la GPU terminen.
     *
     * Util para garantizar que no hay trabajo en vuelo antes de liberar
     * recursos GPU manualmente. Equivale a un vkDeviceWaitIdle interno.
     *
     * @throws std::logic_error si el motor no esta inicializado.
     */
    void wait() {
        requireInitialized( "EngineAPI::wait" );
        _engine.wait();
    }

    /**
     * @brief Renderiza el frame actual y lo presenta en pantalla.
     *
     * Recoge el estado actual de la Scene (entidades, luces, camara),
     * ejecuta las passes de renderizado y presenta el resultado en la ventana.
     * Debe llamarse en cada iteracion del bucle principal de la aplicacion.
     *
     * @throws std::logic_error si el motor no esta inicializado.
     */
    void drawFrame() {
        requireInitialized( "EngineAPI::drawFrame" );
        _engine.drawFrame();
    }

    /**
     * @brief Notifica al motor un evento de ventana.
     *
     * La aplicacion debe traducir los eventos nativos del backend de ventana
     * (SDL, GLFW, Win32...) al tipo WindowEvent y entregarlos aqui. El motor
     * reacciona internamente segun el tipo (por ejemplo, recrea el swapchain
     * ante un WindowEventType::Resized).
     *
     * @param event Evento de ventana a procesar.
     * @throws std::logic_error si el motor no esta inicializado.
     * @see WindowEvent
     * @see WindowEventType
     */
    void handleWindowEvent( const WindowEvent& event ) {
        requireInitialized( "EngineAPI::handleWindowEvent" );
        _engine.handleWindowEvent( event );
    }

    /**
     * @brief Devuelve la referencia a la Scene principal del motor.
     *
     * La Scene permite crear y destruir entidades renderizables y luces, y
     * acceder a la camara principal.
     *
     * @return Referencia a la Scene. Su ciclo de vida esta ligado al motor.
     * @throws std::logic_error si el motor no esta inicializado.
     * @see Scene
     */
    Scene& getScene() {
        requireInitialized( "EngineAPI::getScene" );
        return *_scene;
    }

    /**
     * @brief Devuelve la referencia al ResourceManager del motor.
     *
     * El ResourceManager permite crear, liberar y consultar meshes, texturas
     * y materiales.
     *
     * @return Referencia al ResourceManager. Su ciclo de vida esta ligado al motor.
     * @throws std::logic_error si el motor no esta inicializado.
     * @see ResourceManager
     */
    ResourceManager& getResourceManager() {
        requireInitialized( "EngineAPI::getResourceManager" );
        return *_resources;
    }

private:
    /**
     * @brief Verifica que el motor esta inicializado; lanza std::logic_error si no.
     * @param callSite Nombre del metodo que realiza la comprobacion (para el mensaje de error).
     */
    void requireInitialized( const char* callSite ) const {
        if (!_initialized || _scene == nullptr) {
            throw std::logic_error( std::string( callSite ) + ": engine is not initialized" );
        }
    }

private:
    RenderEngine     _engine;
    Scene*           _scene;
    ResourceManager* _resources;
    bool             _initialized;
};
