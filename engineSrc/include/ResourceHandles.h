#pragma once

/**
 * @file ResourceHandles.h
 * @brief Handles opacos para identificar recursos gestionados por ResourceManager.
 *
 * Cada handle encapsula el par (index, generation) que identifica un slot
 * en el ResourceManager. La generacion 0 esta reservada como sentinel de handle
 * nunca inicializado.
 *
 * Los handles son valores ligeros (copiables) pero opacos: la aplicacion no
 * puede fabricar handles validos ni mutar su estado interno. Solo ResourceManager
 * puede construirlos e invalidarlos.
 *
 * API minima publica por handle:
 * - Constructor por defecto: produce un handle nulo/invalido.
 * - isValid(): valida solo el estado interno del token (no consulta al ResourceManager).
 * - operator==: comparacion de identidad tecnica.
 */

#include <cstdint>
#include <limits>

// ---------------------------------------------------------------------------
// Constantes de invalidez
// ---------------------------------------------------------------------------

/// @brief Valor de indice reservado para representar un slot invalido.
constexpr uint32_t INVALID_HANDLE_INDEX      = std::numeric_limits<uint32_t>::max();

/// @brief Valor de generacion reservado para representar un handle nunca inicializado.
constexpr uint32_t INVALID_HANDLE_GENERATION = 0;

class ResourceManager;

// ---------------------------------------------------------------------------
// TextureHandle
// ---------------------------------------------------------------------------

/**
 * @brief Handle opaco que identifica una textura en el ResourceManager.
 *
 * Un TextureHandle valido se obtiene exclusivamente mediante
 * ResourceManager::createTexture(). Una vez liberada la textura con
 * ResourceManager::releaseTexture(), el handle queda stale y cualquier
 * intento de usarlo producira un ResourceException(StaleHandle).
 *
 * El handle es copiable: puede almacenarse en MaterialCreateInfo para
 * referenciar texturas desde materiales.
 *
 * @see ResourceManager::createTexture()
 * @see ResourceManager::releaseTexture()
 */
class TextureHandle {
public:
    /// @brief Construye un handle invalido (nulo). No representa ninguna textura.
    TextureHandle() = default;

    /**
     * @brief Indica si el handle tiene un valor interno valido.
     *
     * Devuelve @c true cuando el handle fue emitido por ResourceManager y sus
     * campos internos son coherentes. No garantiza que la textura siga viva
     * en el ResourceManager (puede haber sido liberada). Para verificar la
     * vigencia real, usa ResourceManager::tryGetTextureHandle().
     *
     * @return @c true si el handle no es nulo, @c false en caso contrario.
     */
    bool isValid() const {
        return _index != INVALID_HANDLE_INDEX && _generation != INVALID_HANDLE_GENERATION;
    }

    /**
     * @brief Compara dos handles por identidad tecnica.
     *
     * Dos handles son iguales si y solo si apuntan al mismo slot con la misma
     * generacion. Un handle nulo solo es igual a otro handle nulo.
     *
     * @param other Handle con el que comparar.
     * @return @c true si ambos handles identifican el mismo recurso.
     */
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

// ---------------------------------------------------------------------------
// MeshHandle
// ---------------------------------------------------------------------------

/**
 * @brief Handle opaco que identifica una malla en el ResourceManager.
 *
 * Un MeshHandle valido se obtiene exclusivamente mediante
 * ResourceManager::createMesh(). Una vez liberada la malla con
 * ResourceManager::releaseMesh(), el handle queda stale.
 *
 * El handle es copiable y puede pasarse a RenderEntityHandle::setMesh()
 * para asociar geometria a una entidad de la escena.
 *
 * @see ResourceManager::createMesh()
 * @see ResourceManager::releaseMesh()
 * @see RenderEntityHandle::setMesh()
 */
class MeshHandle {
public:
    /// @brief Construye un handle invalido (nulo). No representa ninguna malla.
    MeshHandle() = default;

    /**
     * @brief Indica si el handle tiene un valor interno valido.
     * @return @c true si el handle no es nulo, @c false en caso contrario.
     * @see TextureHandle::isValid() para una descripcion completa del contrato.
     */
    bool isValid() const {
        return _index != INVALID_HANDLE_INDEX && _generation != INVALID_HANDLE_GENERATION;
    }

    /**
     * @brief Compara dos handles por identidad tecnica.
     * @param other Handle con el que comparar.
     * @return @c true si ambos handles identifican la misma malla.
     */
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

// ---------------------------------------------------------------------------
// MaterialHandle
// ---------------------------------------------------------------------------

/**
 * @brief Handle opaco que identifica un material en el ResourceManager.
 *
 * Un MaterialHandle valido se obtiene exclusivamente mediante
 * ResourceManager::createMaterial(). Una vez liberado el material con
 * ResourceManager::releaseMaterial(), el handle queda stale.
 *
 * El handle es copiable y puede pasarse a RenderEntityHandle::setMaterial()
 * para asociar apariencia a una entidad de la escena.
 *
 * @see ResourceManager::createMaterial()
 * @see ResourceManager::releaseMaterial()
 * @see RenderEntityHandle::setMaterial()
 */
class MaterialHandle {
public:
    /// @brief Construye un handle invalido (nulo). No representa ningun material.
    MaterialHandle() = default;

    /**
     * @brief Indica si el handle tiene un valor interno valido.
     * @return @c true si el handle no es nulo, @c false en caso contrario.
     * @see TextureHandle::isValid() para una descripcion completa del contrato.
     */
    bool isValid() const {
        return _index != INVALID_HANDLE_INDEX && _generation != INVALID_HANDLE_GENERATION;
    }

    /**
     * @brief Compara dos handles por identidad tecnica.
     * @param other Handle con el que comparar.
     * @return @c true si ambos handles identifican el mismo material.
     */
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
