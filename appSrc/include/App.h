#pragma once

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/vec4.hpp>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/hash.hpp>



#include<SDL2/SDL_events.h>


#include <chrono>

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <optional>
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <glm/glm.hpp>
#include <array>
#include "VulkanWindow.h"
#include "VulkanDevice.h"
#include "Texture.h"
#include "Utils.h"
#include "BufferObjectsData.h"
#include "Mesh.h"

#include "Camera.h"

#include "RenderEngine.h"


const std::string MODEL_PATH = "./mesh/personaje.obj";
const std::string MODEL_PATH2 = "./mesh/plano.obj";
const std::string MODEL_PATH3 = "./mesh/esfera.obj";
const std::string TEXTURE_PATH = "./textures/ninjaTexture.png";
const std::string TEXTURE2_PATH = "./textures/whitePixel.jpg";
const std::string TEXTURE3_PATH = "./textures/koreano.jpeg";

class App
{
public:

    App();

    void run() {
        _engine.init();
        mainLoop();
        _engine.wait();
        freeObjects();
        _engine.cleanup();
    }

    static void ErrorCallback(int, const char* err_str)
    {
        std::cout << "GLFW Error: " << err_str << std::endl;
    }

private:

    void mainLoop();

    void start();

    void update();
    
    void drawFrame();

    void loadModels();

    void freeObjects();
   
private:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    std::vector<RenderObject> objects;

    glm::vec3 _moveDir;

    RenderEngine _engine;

    Camera* _mainCamera;

};
