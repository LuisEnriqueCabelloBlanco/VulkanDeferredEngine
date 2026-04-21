#pragma once
#include<vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <array>
#include <cstdint>
#include <limits>


enum class LightType : std::int32_t
{
    DIRECTIONAL = 0,//Rayos paralelos
    POINT = 1,//rayos poryectados radialmente desde un punto
    SPOTLIGHT = 2 //rayos proyectados en cono desde un punto
};

static_assert( sizeof( LightType ) == sizeof( std::int32_t ), "LightType must remain 32-bit for GPU buffer compatibility" );

struct alignas(16) Light {
    glm::vec3 pos_dir;
    LightType type;
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

constexpr uint32_t INVALID_TEXTURE_HANDLE = std::numeric_limits<uint32_t>::max();
constexpr uint32_t INVALID_MATERIAL_HANDLE = std::numeric_limits<uint32_t>::max();
constexpr uint32_t INVALID_MESH_HANDLE = std::numeric_limits<uint32_t>::max();
constexpr uint32_t INVALID_HANDLE_GENERATION = 0;

struct TextureHandle {
    uint32_t id = INVALID_TEXTURE_HANDLE;
    uint32_t generation = INVALID_HANDLE_GENERATION;

    inline bool isValid() const {
        return id != INVALID_TEXTURE_HANDLE && generation != INVALID_HANDLE_GENERATION;
    }
};

struct MaterialHandle {
    uint32_t id = INVALID_MATERIAL_HANDLE;
    uint32_t generation = INVALID_HANDLE_GENERATION;

    inline bool isValid() const {
        return id != INVALID_MATERIAL_HANDLE && generation != INVALID_HANDLE_GENERATION;
    }
};

struct MaterialDesc {
    glm::vec4 baseColor = glm::vec4( 1.0f );
    float metallic = 0.0f;
    float roughtness = 1.0f;
    TextureHandle baseColorTexture;
    TextureHandle normalTexture;
};

struct MeshHandle {
    uint32_t id = INVALID_MESH_HANDLE;
    uint32_t generation = INVALID_HANDLE_GENERATION;

    inline bool isValid() const {
        return id != INVALID_MESH_HANDLE && generation != INVALID_HANDLE_GENERATION;
    }
};

struct RenderObject {
    MeshHandle mesh;
    glm::mat4 modelMatrix;
    MaterialHandle material;
};


struct AABBModel {
    glm::vec3 minBound;
    glm::vec3 maxBound;
    glm::mat4 modelMat;
};
