#pragma once

#include <iostream>
#include <string>
#include <vector>

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
    struct LoadedMeshData {
        std::string    meshName;
        MeshHandle     mesh;
        MaterialHandle material;
        bool           hasMaterial = false;
    };

    EngineAPI          _engine;
    CameraController   _cameraController;

    // Recursos cargados desde assets/meshes (Mesh_*.obj + .mtl)
    std::vector<LoadedMeshData>      _loadedMeshes;
    std::vector<RenderEntityHandle>  _sceneEntities;

    // Recurso auxiliar para visualizar la posicion de point lights
    MeshHandle     _lightMarkerMesh;
    MaterialHandle _lightMarkerMat;

    // Suelo gigante
    MeshHandle     _floorMesh;
    MaterialHandle _floorMat;

    // Handle de la luz principal, para proyectar sombras
    LightEntityHandle _mainLight;
};
