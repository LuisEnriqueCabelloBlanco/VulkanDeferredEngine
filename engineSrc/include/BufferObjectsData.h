#pragma once

// ---------------------------------------------------------------------------
// BufferObjectsData.h
//
// Estructuras internas del renderer. Este header es exclusivo de la capa de
// renderizado: RenderEngine, VulkanDevice y los sistemas de culling son los
// unicos consumidores legitimos.
//
// La aplicacion nunca incluye este header directamente; accede a los recursos
// a traves de ResourceHandles.h y a las entidades a traves de Scene.h /
// RenderEntityHandle.h / LightEntityHandle.h.
// ---------------------------------------------------------------------------

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/hash.hpp>

#include <cstdint>

#include "ResourceHandles.h"

// ---------------------------------------------------------------------------
// Camara y iluminacion global
// ---------------------------------------------------------------------------

struct ViewProjectionData {
    glm::mat4 view;
    glm::mat4 proj;
};

struct alignas(16) GlobalLighting {
    glm::vec3 eyePos;
    float     ambientVal;
};

enum class LightType : int32_t {
    Directional = 0,
    Point       = 1,
    Spotlight   = 2,
};

static_assert( sizeof( LightType ) == sizeof( int32_t ),
               "LightType must remain 32-bit for GPU buffer compatibility" );

// ---------------------------------------------------------------------------
// LightObject
//
// Representacion GPU de una luz. El layout con alignas(16) es un contrato
// con el shader: cualquier cambio aqui debe reflejarse en el GLSL/HLSL.
//
// LightType vive en este mismo header y su valor int32_t es consumido
// directamente por el shader como un campo de 32 bits.
//
// La aplicacion no manipula LightObject directamente; usa LightDesc para
// describir una luz y LightEntityHandle para mutarla. Scene traduce esos
// datos a LightObject en el slot correspondiente.
// ---------------------------------------------------------------------------

struct alignas(16) LightObject {
    glm::vec3 posOrDir;     // Posicion (Point/Spot) o direccion normalizada (Directional).
    LightType type;         // int32_t en GPU.
    glm::vec3 color;
    float     intensity;
    alignas(16) float range;
};

// ---------------------------------------------------------------------------
// RenderObject
//
// Unidad que el renderer consume cada frame para dibujar geometria.
// Contiene handles en lugar de punteros para que el renderer los resuelva
// contra el ResourceManager en el momento de dibujar.
//
// Orden de campos: los dos handles (8 bytes cada uno) antes de la mat4
// (alineamiento 16) para evitar padding implicito.
// ---------------------------------------------------------------------------

struct RenderObject {
    MeshHandle     mesh{};
    MaterialHandle material{};
    glm::mat4      modelMatrix{ 1.0f };
};

// Layout usado por push constants en fragment shader.
struct alignas(16) MaterialData {
    glm::vec4 baseColor = glm::vec4( 1.0f );
    float metallic = 0.0f;
    float roughness = 1.0f;
    int textureIndex = -1;
    int normalTextureIndex = -1;
};

// ---------------------------------------------------------------------------
// Culling
// ---------------------------------------------------------------------------

struct AABBModel {
    glm::vec3 minBound;
    glm::vec3 maxBound;
    glm::mat4 modelMat;
};
