#pragma once

#include <iostream>

#include "EngineAPI.h"
#include "CameraController.h"
#include "WindowHandler.h"

/**
 * Aplicacion minima de ejemplo para testers.
 *
 * Monta una escena con un cubo en el origen, un plano de suelo,
 * una luz direccional y dos point lights de relleno. El bucle
 * principal delega el input de camara en CameraController y los
 * eventos de ventana en WindowHandler.
 *
 * Para extender la aplicacion:
 *   - Carga mallas, texturas y materiales en loadResources().
 *   - Crea entidades y luces en populateScene() usando los handles
 *     guardados como miembros durante loadResources().
 *   - Aniade logica de juego por frame en update().
 */
class App {
public:
    App() = default;

    /**
     * Inicializa el motor, ejecuta el bucle principal y limpia al salir.
     * @return true si termino sin errores, false si hubo una excepcion fatal.
     */
    bool run();

private:
    /** Crea mallas, texturas y materiales en el ResourceManager.
     *  Los handles se guardan como miembros para que populateScene() los use. */
    void loadResources();

    /** Crea entidades y luces en la Scene usando los handles de loadResources().
     *  Configura tambien la camara inicial. */
    void populateScene();

    void mainLoop();
    void update( float deltaTime );
    void freeScene();
    void safeCleanup( bool& initialized ) noexcept;

private:
    EngineAPI          _engine;
    CameraController   _cameraController;

    // Handles de recursos (rellenados en loadResources)
    MeshHandle     _cubeMesh;
    MeshHandle     _planeMesh;
    MaterialHandle _cubeMat;
    MaterialHandle _planeMat;

    // Handles de entidades (rellenados en populateScene)
    RenderEntityHandle _cubeEntity;
    RenderEntityHandle _planeEntity;

    // Handle de la luz principal, para proyectar sombras
    LightEntityHandle _mainLight;
};
