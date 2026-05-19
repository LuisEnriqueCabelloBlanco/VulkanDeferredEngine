#pragma once

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/vec4.hpp>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/hash.hpp>


#include <iostream>
#include <string>
#include <vector>
#include<random>

#include <glm/glm.hpp>
#include <SDL2/SDL_events.h>

#include "EngineAPI.h"
#include "ResourcePaths.h"
#include "BenchmarkScene.h"



class App
{
public:

    App();

    bool run();

    static void ErrorCallback(int, const char* err_str)
    {
        std::cout << "GLFW Error: " << err_str << std::endl;
    }

private:

    void mainLoop();

    void start();

    void update();

    void loadModels();

    void addLighting();

    void freeObjects();

    void safeCleanup( bool& initialized ) noexcept;

private:

    void createCandleGrid(Scene& scene, const MeshHandle& mesh, const MaterialHandle& mat);

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    glm::vec3 _moveDir;

    EngineAPI _engine;

    CameraHandle* _mainCamera;

    float _deltaTime;

    int _counter =0;

    float timeacum = 0;

    RenderEntityHandle _triangleEntity;
    RenderEntityHandle _characterEntity;
    RenderEntityHandle _sphereLeftEntity;
    RenderEntityHandle _sphereRightEntity;
    std::vector<RenderEntityHandle> _candleEntity;

    bool _axisInput[4] = { false, false, false,false };

    float _movementSpeed = 1.f;

    BenchmarkScene _benchScene;

    //random tools
    std::mt19937 _gen;
    std::random_device _rd;  // a seed source for the random number engine
};
