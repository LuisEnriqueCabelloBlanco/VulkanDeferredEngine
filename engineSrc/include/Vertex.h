#pragma once

/**
 * @file Vertex.h
 * @brief Definicion del vertice usado en la pipeline de renderizado y su hash.
 */

#include <vulkan/vulkan_core.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <array>

/**
 * @brief Vertice de la geometria renderizable.
 *
 * Contiene todos los atributos necesarios para la pipeline de renderizado
 * con soporte de iluminacion por normales y mapas de normales (normal mapping).
 *
 * El layout de atributos Vulkan se obtiene a traves de los metodos estaticos
 * getBindingDescription() y getAttributeDescriptions(), que deben usarse al
 * crear la pipeline grafica.
 *
 * Localizaciones de atributos en el shader:
 * - location 0: pos       (vec3)
 * - location 1: color     (vec3)
 * - location 2: texCoord  (vec2)
 * - location 3: normal    (vec3)
 * - location 4: tangent   (vec3)
 */
struct Vertex {
    glm::vec3 pos;      ///< Posicion del vertice en espacio local.
    glm::vec3 color;    ///< Color por vertice (puede ignorarse si hay textura).
    glm::vec2 texCoord; ///< Coordenadas UV de textura.
    glm::vec3 normal;   ///< Normal del vertice en espacio local.
    glm::vec3 tangent;  ///< Tangente del vertice, usada para normal mapping.

    /**
     * @brief Devuelve el descriptor de binding de vertice para Vulkan.
     *
     * Configura el binding 0 con stride sizeof(Vertex) y tasa de entrada
     * por vertice (VK_VERTEX_INPUT_RATE_VERTEX).
     *
     * @return VkVertexInputBindingDescription listo para usar en la pipeline.
     */
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof( Vertex );
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    /**
     * @brief Devuelve los descriptores de atributo de vertice para Vulkan.
     *
     * Mapea los cinco campos del struct a las localizaciones del vertex shader:
     * pos (0), color (1), texCoord (2), normal (3), tangent (4).
     *
     * @return Array de 5 VkVertexInputAttributeDescription.
     */
    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

        // Posicion
        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof( Vertex, pos );

        // Color
        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof( Vertex, color );

        // Coordenadas UV
        attributeDescriptions[2].binding  = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset   = offsetof( Vertex, texCoord );

        // Normal
        attributeDescriptions[3].binding  = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset   = offsetof( Vertex, normal );

        // Tangente
        attributeDescriptions[4].binding  = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset   = offsetof( Vertex, tangent );

        return attributeDescriptions;
    }

    /**
     * @brief Compara dos vertices por igualdad de todos sus campos.
     *
     * Util para la deduplicacion de vertices durante la carga de mallas.
     * Nota: la tangente no participa en la comparacion.
     *
     * @param other Vertice con el que comparar.
     * @return @c true si pos, color, texCoord y normal son iguales.
     */
    bool operator==( const Vertex& other ) const {
        return pos == other.pos && color == other.color
            && texCoord == other.texCoord && normal == other.normal;
    }
};

/// @brief Especializacion de std::hash para Vertex. Permite usar Vertex como clave en contenedores hash.
namespace std {
template<> struct hash<Vertex> {
    size_t operator()( Vertex const& vertex ) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
            (hash<glm::vec3>()(vertex.normal) << 1);
    }
};
}
