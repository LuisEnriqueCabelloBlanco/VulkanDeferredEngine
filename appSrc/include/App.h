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

#include <glm/glm.hpp>
#include <SDL2/SDL_events.h>

#include "EngineAPI.h"


const std::string MODEL_PATH = "./mesh/personaje.obj";
const std::string MODEL_PATH2 = "./mesh/plano.obj";
const std::string MODEL_PATH3 = "./mesh/esfera.obj";
const std::string MODEL_PATH4 = "./mesh/planoSinColor.obj";
const std::string TEXTURE_PATH = "./textures/ninjaTexture.png";
const std::string TEXTURE2_PATH = "./textures/whitePixel.jpg";
const std::string TEXTURE3_PATH = "./textures/pedro.jpeg";
const std::string NORMAL_TEXTURE_PATH = "./textures/Ch45_1001_Normal.png";
const std::string WALL_TEXTURE_PATH = "./textures/MuroColor.jpg";
const std::string WALL_NORMAL_TEXTURE_PATH = "./textures/MuroNormal.jpg";


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
};
