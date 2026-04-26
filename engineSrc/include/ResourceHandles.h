#pragma once

#include <cstdint>
#include <limits>

// ---------------------------------------------------------------------------
// Constantes de invalidez
// ---------------------------------------------------------------------------

constexpr uint32_t INVALID_HANDLE_INDEX      = std::numeric_limits<uint32_t>::max();
constexpr uint32_t INVALID_HANDLE_GENERATION = 0;

// ---------------------------------------------------------------------------
// Handles de recursos
//
// Cada handle encapsula el par (index, generation) que identifica un slot en
// el ResourceManager. La generacion 0 esta reservada como sentinel de handle
// nunca inicializado.
//
// Los handles son valores planos (copiables). El ResourceManager es el unico
// que puede crearlos y el unico que puede invalidarlos al liberar el recurso.
// ---------------------------------------------------------------------------

struct TextureHandle {
    uint32_t index      = INVALID_HANDLE_INDEX;
    uint32_t generation = INVALID_HANDLE_GENERATION;

    bool isValid() const {
        return index != INVALID_HANDLE_INDEX && generation != INVALID_HANDLE_GENERATION;
    }
};

struct MeshHandle {
    uint32_t index      = INVALID_HANDLE_INDEX;
    uint32_t generation = INVALID_HANDLE_GENERATION;

    bool isValid() const {
        return index != INVALID_HANDLE_INDEX && generation != INVALID_HANDLE_GENERATION;
    }
};

