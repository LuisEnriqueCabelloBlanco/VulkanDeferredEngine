#pragma once

#include <cstdint>
#include <limits>

#include <glm/glm.hpp>

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

struct MeshHandle {
    uint32_t id = INVALID_MESH_HANDLE;
    uint32_t generation = INVALID_HANDLE_GENERATION;

    inline bool isValid() const {
        return id != INVALID_MESH_HANDLE && generation != INVALID_HANDLE_GENERATION;
    }
};

struct MaterialDesc {
    glm::vec4 baseColor = glm::vec4( 1.0f );
    float metallic = 0.0f;
    float roughtness = 1.0f;
    TextureHandle baseColorTexture;
    TextureHandle normalTexture;
};
