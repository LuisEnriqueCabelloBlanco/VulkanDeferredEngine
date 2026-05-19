#pragma once

/**
 * @file BufferObjectsData.h
 * @brief Estructuras internas del renderer.
 *
 * Este header es exclusivo de la capa de renderizado: RenderEngine, VulkanDevice
 * y los sistemas de culling son los unicos consumidores legitimos.
 *
 * La aplicacion nunca incluye este header directamente; accede a los recursos
 * a traves de ResourceHandles.h y a las entidades a traves de Scene.h.
 * La unica excepcion es LightType, que la aplicacion necesita conocer para
 * crear luces a traves de Scene::createLight() y LightEntityHandle::setType().
 */

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

/// @cond INTERNAL
struct ViewProjectionData {
    glm::mat4 view;
    glm::mat4 proj;
};

struct alignas(16) GlobalLighting {
    glm::vec3 eyePos;
    float     ambientVal;
    uint32_t mainLightIndex;
};
/// @endcond

// ---------------------------------------------------------------------------
// LightType
// ---------------------------------------------------------------------------

/**
 * @brief Tipo de luz soportado por el motor.
 *
 * Determina como el renderer interpreta el campo @c posOrDir del slot de luz
 * y como calcula la contribucion luminosa en el shader:
 *
 * - @b Directional: luz infinitamente lejana. @c posOrDir es una direccion
 *   normalizada que apunta HACIA la fuente. No tiene rango ni atenuacion.
 *   Es el tipo valido para designar una luz como sombra principal (main light).
 *
 * - @b Point: luz omnidireccional con origen en @c posOrDir. La atenuacion
 *   depende del campo @c range del handle.
 *
 * - @b Spotlight: cono de luz con origen en @c posOrDir. La apertura y la
 *   atenuacion dependen del campo @c range del handle.
 *
 * El valor subyacente es @c int32_t para garantizar compatibilidad directa
 * con el campo de 32 bits del buffer de luces en la GPU.
 *
 * @see Scene::createLight()
 * @see LightEntityHandle::setType()
 */
enum class LightType : int32_t {
    Directional = 0, ///< Luz direccional global sin atenuacion por distancia.
    Point       = 1, ///< Luz puntual omnidireccional con atenuacion por rango.
    Spotlight   = 2, ///< Luz de cono con atenuacion por rango.
};

static_assert( sizeof( LightType ) == sizeof( int32_t ),
               "LightType must remain 32-bit for GPU buffer compatibility" );

// ---------------------------------------------------------------------------
// Estructuras internas del renderer — no forman parte de la API publica
// ---------------------------------------------------------------------------

/// @cond INTERNAL

/**
 * Representacion GPU de una luz. El layout con alignas(16) es un contrato
 * con el shader: cualquier cambio aqui debe reflejarse en el GLSL/HLSL.
 *
 * La aplicacion no manipula LightObject directamente; usa LightEntityHandle
 * para mutar la luz y Scene traduce esos datos a LightObject en el slot
 * correspondiente.
 */
struct alignas(16) LightObject {
    glm::vec3 posOrDir;     ///< Posicion (Point/Spot) o direccion normalizada (Directional).
    LightType type;         ///< Tipo de luz (int32_t en GPU).
    glm::vec3 color;
    float     intensity;
    alignas(16) float range;
};

/**
 * Unidad que el renderer consume cada frame para dibujar geometria.
 * Contiene handles en lugar de punteros para que el renderer los resuelva
 * contra el ResourceManager en el momento de dibujar.
 *
 * Orden de campos: los dos handles (8 bytes cada uno) antes de la mat4
 * (alineamiento 16) para evitar padding implicito.
 */
struct RenderObject {
    MeshHandle     mesh{};
    MaterialHandle material{};
    glm::mat4      modelMatrix{ 1.0f };
};

/// Layout usado por push constants en fragment shader.
struct alignas(16) MaterialData {
    glm::vec4 baseColor = glm::vec4( 1.0f );
    float metallic = 0.0f;
    float roughness = 1.0f;
    int textureIndex = -1;
    int normalTextureIndex = -1;
};

/// AABB en espacio de modelo para el sistema de culling.
struct AABBModel {
    glm::vec3 minBound;
    glm::vec3 maxBound;
    glm::mat4 modelMat;
};

/// @endcond
