#pragma once

#include <cstdint>
#include <limits>

// ---------------------------------------------------------------------------
// Constantes de invalidez
// ---------------------------------------------------------------------------

constexpr uint32_t INVALID_HANDLE_INDEX      = std::numeric_limits<uint32_t>::max();
constexpr uint32_t INVALID_HANDLE_GENERATION = 0;

class ResourceManager;

// ---------------------------------------------------------------------------
// Handles de recursos
//
// Cada handle encapsula el par (index, generation) que identifica un slot en
// el ResourceManager. La generacion 0 esta reservada como sentinel de handle
// nunca inicializado.
//
// Los handles son valores ligeros (copiables) pero opacos: la aplicacion no
// puede fabricar handles validos ni mutar su estado interno.
//
// API minima publica por handle:
// - ctor por defecto: representa handle nulo/invalido.
// - isValid(): valida solo el estado interno del token.
// - operator==: comparacion de identidad tecnica.
//
// ResourceManager es friend y el unico autorizado para construir handles
// validos (index/generation) e invalidarlos via bump de generation al liberar.
// ---------------------------------------------------------------------------

class TextureHandle {
public:
    TextureHandle() = default;

    bool isValid() const {
        return _index != INVALID_HANDLE_INDEX && _generation != INVALID_HANDLE_GENERATION;
    }

    bool operator==( const TextureHandle& other ) const {
        return _index == other._index && _generation == other._generation;
    }

private:
    friend class ResourceManager;

    TextureHandle( uint32_t index, uint32_t generation )
        : _index( index ), _generation( generation ) {
    }

    uint32_t _index      = INVALID_HANDLE_INDEX;
    uint32_t _generation = INVALID_HANDLE_GENERATION;
};

class MeshHandle {
public:
    MeshHandle() = default;

    bool isValid() const {
        return _index != INVALID_HANDLE_INDEX && _generation != INVALID_HANDLE_GENERATION;
    }

    bool operator==( const MeshHandle& other ) const {
        return _index == other._index && _generation == other._generation;
    }

private:
    friend class ResourceManager;

    MeshHandle( uint32_t index, uint32_t generation )
        : _index( index ), _generation( generation ) {
    }

    uint32_t _index      = INVALID_HANDLE_INDEX;
    uint32_t _generation = INVALID_HANDLE_GENERATION;
};

class MaterialHandle {
public:
    MaterialHandle() = default;

    bool isValid() const {
        return _index != INVALID_HANDLE_INDEX && _generation != INVALID_HANDLE_GENERATION;
    }

    bool operator==( const MaterialHandle& other ) const {
        return _index == other._index && _generation == other._generation;
    }

private:
    friend class ResourceManager;

    MaterialHandle( uint32_t index, uint32_t generation )
        : _index( index ), _generation( generation ) {
    }

    uint32_t _index      = INVALID_HANDLE_INDEX;
    uint32_t _generation = INVALID_HANDLE_GENERATION;
};

