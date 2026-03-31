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
    DIRECTIONAL,
    POINT
};

struct DirectionalLight {
    glm::vec4 color;
    glm::vec3 direction;
    float intensity;
};

struct PointLight {
    glm::vec4 color;
    glm::vec3 position;
    float intensity;
};

struct alignas(16) Light {
    glm::vec3 pos_dir;
    int type;
    glm::vec3 color;
    float intensity;
    alignas(16) float range;
};

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

//OJITO CON EL PADDING
struct GlobalLighting {
    glm::vec3 eyePos;
    float ambietnVal;
};

struct MaterialData {
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

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof( Vertex );
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
        //posicion
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof( Vertex, pos );
        //color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof( Vertex, color );
        //UV
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        //normales
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof( Vertex, normal );

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof( Vertex, tangent );

        return attributeDescriptions;
    }

    bool operator==( const Vertex& other ) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }
};

//Hash para los vertices
namespace std
{
template<> struct hash<Vertex> {
    size_t operator()( Vertex const& vertex ) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                 (hash<glm::vec3>()(vertex.normal) << 1);
    }
};
}