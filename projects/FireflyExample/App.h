#pragma once

#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "EngineAPI.h"
#include "CameraController.h"
#include "WindowHandler.h"

/**
 * Aplicacion minima de ejemplo para testers.
 *
 * Monta una escena con mallas del escenario y luciernagas luminosas.
 * El bucle principal delega el input de camara en CameraController y los
 * eventos de ventana en WindowHandler.
 *
 * Para extender la aplicacion:
 *   - Carga mallas y materiales en loadResources().
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
    /** Crea mallas y materiales en el ResourceManager.
     *  Los handles se guardan como miembros para que populateScene() los use. */
    void loadResources();

    /** Crea entidades y luces en la Scene usando los handles de loadResources().
     *  Configura tambien la camara inicial. */
    void populateScene();

    void mainLoop();
    void update(float deltaTime);
    void freeScene();
    void safeCleanup(bool& initialized) noexcept;
    void spawnFireflies(Scene& scene);
    void updateFireflies(float deltaTime);

private:
    struct LoadedMeshData {
        std::string    meshName;
        MeshHandle     mesh;
        MaterialHandle material;
        bool           hasMaterial = false;
    };

    struct Firefly {
        LightEntityHandle  light;
        RenderEntityHandle visual;
        glm::vec3          position;
        glm::vec3          target;
        glm::vec3          color;
        float              speed = 0.0f;
    };

    EngineAPI          _engine;
    CameraController   _cameraController;
    std::mt19937       _rng{ std::random_device{}() };

    // Recursos cargados desde assets/meshes (Mesh_*.obj + .mtl)
    std::vector<LoadedMeshData>      _loadedMeshes;
    std::vector<RenderEntityHandle>  _sceneEntities;
    std::vector<Firefly>             _fireflies;
    MeshHandle                       _fireflySphereMesh;
};
