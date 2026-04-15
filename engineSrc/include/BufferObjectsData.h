#pragma once
#include<vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <array>


enum LightType
{
    DIRECTIONAL,//Rayos paralelos
    POINT,//rayos poryectados radialmente desde un punto
    SPOTLIGHT //rayos proyectados en cono desde un punto
};

struct alignas(16) Light {
    glm::vec3 pos_dir;
    int type;
    glm::vec3 color;
    float intensity;
    alignas(16) float range;
};

struct ViewProjectionData {
    glm::mat4 view;
    glm::mat4 proj;
};

struct alignas(16) GlobalLighting {
    glm::vec3 eyePos;
    float ambietnVal;
};

struct alignas(16) MaterialData {
    float metallic;
    float roughtness;
    int texutreIndex;
    int normalTextureIndex;
};

class Mesh;

struct RenderObject {
    Mesh* mesh;
    glm::mat4 modelMatrix;
    MaterialData mat;
};


struct AABBModel {
    glm::vec3 minBound;
    glm::vec3 maxBound;
    glm::mat4 modelMat;
};
