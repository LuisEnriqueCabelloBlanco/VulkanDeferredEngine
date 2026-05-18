#pragma once

/**
 * @file ResourceManager.h
 * @brief Gestion centralizada de meshes, texturas y materiales del motor.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec4.hpp>

#include "BufferObjectsData.h"
#include "ResourceHandles.h"
#include "Mesh.h"
#include "ResourceLimits.h"
#include "Texture.h"
#include "VulkanDevice.h"

// ---------------------------------------------------------------------------
// MaterialCreateInfo
// ---------------------------------------------------------------------------

/**
 * @brief Parametros de creacion de un material PBR.
 *
 * Se pasa a ResourceManager::createMaterial() para describir las propiedades
 * visuales del material. Los campos de textura aceptan handles invalidos
 * (construidos por defecto) para indicar ausencia de textura.
 *
 * Ejemplo de uso:
 * @code
 * MaterialCreateInfo info;
 * info.baseColor        = { 1.f, 0.8f, 0.2f, 1.f };
 * info.metallic         = 0.0f;
 * info.roughness        = 0.5f;
 * info.baseColorTexture = resources.createTexture( "albedo", "albedo.png" );
 * MaterialHandle mat = resources.createMaterial( "gold", info );
 * @endcode
 */
struct MaterialCreateInfo {
    glm::vec4     baseColor = glm::vec4( 1.0f ); ///< Color base en formato RGBA (default: blanco opaco).
    float         metallic  = 0.0f;              ///< Factor metalico en [0, 1] (0 = dielectrico).
    float         roughness = 1.0f;              ///< Factor de rugosidad en [0, 1] (1 = completamente rugoso).
    TextureHandle baseColorTexture{};            ///< Textura de color base. Handle invalido = sin textura.
    TextureHandle normalTexture{};               ///< Mapa de normales. Handle invalido = sin mapa de normales.
};

// ---------------------------------------------------------------------------
// ResourceErrorCode y ResourceException
// ---------------------------------------------------------------------------

/**
 * @brief Codigos de error del ResourceManager.
 *
 * Permiten distinguir el motivo exacto del fallo sin depender del texto
 * del mensaje de error.
 *
 * @see ResourceException
 */
enum class ResourceErrorCode {
    InvalidName,     ///< El nombre esta vacio o contiene caracteres no validos.
    DuplicateName,   ///< Ya existe un recurso con ese nombre en el registro.
    LimitExceeded,   ///< Se ha superado el limite de recursos del tipo correspondiente.
    InvalidHandle,   ///< El handle no fue emitido o nunca fue valido.
    StaleHandle,     ///< El handle fue valido pero el recurso ya fue liberado.
    DependencyInUse, ///< El recurso no puede liberarse porque otros recursos dependen de el.
    LoadFailed,      ///< Fallo al cargar el recurso desde disco o al inicializarlo en GPU.
};

/**
 * @brief Excepcion lanzada por ResourceManager ante operaciones invalidas.
 *
 * Hereda de std::runtime_error. Ademas del mensaje de texto, expone el
 * codigo de error para que el caller pueda reaccionar de forma programatica.
 *
 * @see ResourceErrorCode
 */
class ResourceException : public std::runtime_error {
public:
    /**
     * @brief Construye la excepcion con un codigo y un mensaje descriptivo.
     * @param code    Codigo de error que identifica el tipo de fallo.
     * @param message Descripcion legible del error.
     */
    ResourceException( ResourceErrorCode code, const std::string& message )
        : std::runtime_error( message ), _code( code ) {
    }

    /**
     * @brief Devuelve el codigo de error asociado a la excepcion.
     * @return Codigo que identifica la causa del fallo.
     */
    ResourceErrorCode code() const {
        return _code;
    }

private:
    ResourceErrorCode _code;
};

// ---------------------------------------------------------------------------
// ResourceManager
// ---------------------------------------------------------------------------

/**
 * @brief Punto de propiedad y control de los recursos de alto nivel del motor.
 *
 * Gestiona tres registros independientes: meshes, texturas y materiales.
 * Cada registro usa una tabla de slots con reutilizacion por free-list, un
 * mapa de nombre a slot para garantizar unicidad explicita, y handles con
 * generation para detectar usos stale.
 *
 * Capacidades principales:
 * - Crear recursos por nombre y rechazar duplicados.
 * - Liberar recursos en runtime con validacion de dependencias cruzadas.
 * - Resolver recursos por nombre o por handle de forma no destructiva.
 * - Proteger dependencias basicas entre recursos: una textura referenciada
 *   por un material no puede liberarse hasta que el material sea destruido.
 * - Enumeracion de recursos vivos para debug e inspeccion.
 *
 * El nombre asegura unicidad; el handle identifica el acceso tecnico seguro;
 * el manager es la unica capa que decide si un recurso sigue siendo valido.
 *
 * Se obtiene a traves de EngineAPI::getResourceManager().
 *
 * @see EngineAPI::getResourceManager()
 * @see ResourceHandles.h
 */
class ResourceManager {
public:
    /// @cond INTERNAL
    struct TextureBindingEntry {
        uint32_t index;
        const Texture* texture;
    };
    /// @endcond

    explicit ResourceManager( VulkanDevice& device );

    /**
     * @brief Limpia todo el estado del manager liberando todos los recursos.
     *
     * El orden interno de liberacion es: materiales -> texturas -> meshes,
     * respetando las dependencias entre tipos. Tras la llamada el manager
     * queda en un estado equivalente al inicial.
     *
     * @warning Si clear() falla en una fase intermedia puede dejar el estado
     *          parcialmente limpiado.
     */
    void clear();

    // -------------------------------------------------------------------------
    // Ciclo de vida de Mesh
    // -------------------------------------------------------------------------

    /**
     * @brief Carga una malla desde un fichero y la registra con el nombre dado.
     *
     * @param name Nombre unico de la malla. No puede estar vacio ni repetido.
     * @param path Ruta al fichero de la malla en disco.
     * @return Handle valido que identifica la malla creada.
     * @throws ResourceException(InvalidName)   si @p name es invalido.
     * @throws ResourceException(DuplicateName) si ya existe una malla con ese nombre.
     * @throws ResourceException(LimitExceeded) si se supera ResourceLimits::MAX_MESHES.
     * @throws ResourceException(LoadFailed)    si el fichero no puede cargarse.
     */
    [[nodiscard]] MeshHandle createMesh( const std::string& name,
                                         const std::string& path );

    /**
     * @brief Crea una malla a partir de una lista de vertices sin indices.
     *
     * @param name     Nombre unico de la malla.
     * @param vertices Lista de vertices que definen la geometria.
     * @return Handle valido que identifica la malla creada.
     * @throws ResourceException segun los mismos criterios que la sobrecarga con path.
     */
    [[nodiscard]] MeshHandle createMesh( const std::string& name,
                                         const std::vector<Vertex>& vertices );

    /**
     * @brief Crea una malla indexada a partir de indices y vertices.
     *
     * @param name     Nombre unico de la malla.
     * @param indices  Indices de los triangulos.
     * @param vertices Lista de vertices que definen la geometria.
     * @return Handle valido que identifica la malla creada.
     * @throws ResourceException segun los mismos criterios que la sobrecarga con path.
     */
    [[nodiscard]] MeshHandle createMesh( const std::string& name,
                                         const std::vector<uint32_t>& indices,
                                         const std::vector<Vertex>& vertices );

    /**
     * @brief Libera una malla e invalida su handle.
     *
     * Tras la llamada, el MeshHandle queda stale y cualquier acceso posterior
     * a el producira un error. Si la malla no existe o el handle es stale,
     * lanza ResourceException.
     *
     * @param handle Handle valido de la malla a liberar.
     * @throws ResourceException(InvalidHandle) si el handle no es valido.
     * @throws ResourceException(StaleHandle)   si la malla ya fue liberada.
     */
    void releaseMesh( MeshHandle handle );

    /// @brief Libera todas las mallas registradas en el manager.
    void releaseAllMeshes();

    // -------------------------------------------------------------------------
    // Ciclo de vida de Texture
    // -------------------------------------------------------------------------

    /**
     * @brief Carga una textura desde fichero y la registra con el nombre dado.
     *
     * @param name Nombre unico de la textura.
     * @param path Ruta al fichero de imagen en disco.
     * @return Handle valido que identifica la textura creada.
     * @throws ResourceException(InvalidName)   si @p name es invalido.
     * @throws ResourceException(DuplicateName) si ya existe una textura con ese nombre.
     * @throws ResourceException(LimitExceeded) si se supera ResourceLimits::MAX_TEXTURES.
     * @throws ResourceException(LoadFailed)    si el fichero no puede cargarse.
     */
    [[nodiscard]] TextureHandle createTexture( const std::string& name,
                                               const std::string& path );

    /**
     * @brief Libera una textura e invalida su handle.
     *
     * Rechaza la liberacion si algun material vivo referencia esta textura.
     * En ese caso lanza ResourceException(DependencyInUse). El material debe
     * liberarse primero.
     *
     * @param handle Handle valido de la textura a liberar.
     * @throws ResourceException(DependencyInUse) si hay materiales que dependen de la textura.
     * @throws ResourceException(InvalidHandle)   si el handle no es valido.
     * @throws ResourceException(StaleHandle)     si la textura ya fue liberada.
     */
    void releaseTexture( TextureHandle handle );

    /**
     * @brief Libera todas las texturas registradas en el manager.
     *
     * Requiere que no existan materiales vivos que referencien texturas.
     * Si los hay, lanza ResourceException(DependencyInUse).
     *
     * @throws ResourceException(DependencyInUse) si existen materiales con texturas activas.
     */
    void releaseAllTextures();

    // -------------------------------------------------------------------------
    // Ciclo de vida de Material
    // -------------------------------------------------------------------------

    /**
     * @brief Crea un material PBR y lo registra con el nombre dado.
     *
     * Valida que las texturas referenciadas en @p material sean handles validos
     * (o handles invalidos si se quiere omitir la textura). Las texturas deben
     * haber sido creadas previamente en este mismo ResourceManager.
     *
     * @param name     Nombre unico del material.
     * @param material Descripcion del material a crear.
     * @return Handle valido que identifica el material creado.
     * @throws ResourceException(InvalidName)   si @p name es invalido.
     * @throws ResourceException(DuplicateName) si ya existe un material con ese nombre.
     * @throws ResourceException(LimitExceeded) si se supera ResourceLimits::MAX_MATERIALS.
     * @throws ResourceException(InvalidHandle) si alguna textura referenciada no es valida.
     * @throws ResourceException(StaleHandle)   si alguna textura referenciada fue liberada.
     */
    [[nodiscard]] MaterialHandle createMaterial( const std::string& name,
                                                  const MaterialCreateInfo& material );

    /**
     * @brief Libera un material e invalida su handle.
     *
     * @param handle Handle valido del material a liberar.
     * @throws ResourceException(InvalidHandle) si el handle no es valido.
     * @throws ResourceException(StaleHandle)   si el material ya fue liberado.
     */
    void releaseMaterial( MaterialHandle handle );

    /// @brief Libera todos los materiales registrados en el manager.
    void releaseAllMaterials();

    // -------------------------------------------------------------------------
    // Lookup por nombre
    // -------------------------------------------------------------------------

    /**
     * @brief Busca una malla por nombre y devuelve su handle.
     *
     * No lanza excepciones. Si la malla no existe o no esta ocupada, devuelve
     * un MeshHandle construido por defecto (invalido).
     *
     * @param name Nombre de la malla a buscar.
     * @return Handle valido si existe, handle invalido si no.
     */
    MeshHandle     tryGetMeshHandle    ( const std::string& name ) const;

    /**
     * @brief Busca una textura por nombre y devuelve su handle.
     *
     * @param name Nombre de la textura a buscar.
     * @return Handle valido si existe, handle invalido si no.
     */
    TextureHandle  tryGetTextureHandle ( const std::string& name ) const;

    /**
     * @brief Busca un material por nombre y devuelve su handle.
     *
     * @param name Nombre del material a buscar.
     * @return Handle valido si existe, handle invalido si no.
     */
    MaterialHandle tryGetMaterialHandle( const std::string& name ) const;

    // -------------------------------------------------------------------------
    // Enumeracion de recursos vivos
    // -------------------------------------------------------------------------

    /**
     * @brief Devuelve los nombres de todas las mallas actualmente registradas.
     *
     * Util para herramientas de debug, inspectores y serializacion.
     * El orden de los elementos no esta garantizado.
     *
     * @return Vector de nombres de mallas activas.
     */
    std::vector<std::string> getMeshNames()     const;

    /**
     * @brief Devuelve los nombres de todas las texturas actualmente registradas.
     *
     * @return Vector de nombres de texturas activas.
     */
    std::vector<std::string> getTextureNames()  const;

    /**
     * @brief Devuelve los nombres de todos los materiales actualmente registrados.
     *
     * @return Vector de nombres de materiales activos.
     */
    std::vector<std::string> getMaterialNames() const;

private:

    struct MeshSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        std::unique_ptr<Mesh> resource;
    };

    struct TextureSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        std::unique_ptr<Texture> resource;
    };

    struct MaterialSlot {
        uint32_t generation = 1;
        bool occupied = false;
        std::string name;
        MaterialData data{};
    };

    friend class RenderEngine;

    void setTextureBindingsChangedCallback( std::function<void()> callback );
    void notifyTextureBindingsChanged() const;
    std::vector<TextureBindingEntry> getLiveTextureEntries() const;

    friend class CullManager;

    const Mesh*         tryGetMesh    ( MeshHandle     handle ) const;
    const Texture*      tryGetTexture ( TextureHandle  handle ) const;
    const MaterialData* tryGetMaterial( MaterialHandle handle ) const;

    static void bumpGeneration( uint32_t& generation );
    static void validateResourceName( const std::string& name, const char* callSite );
    static std::string makeDuplicateNameMessage( const char* callSite, const std::string& name );

    MeshHandle     makeMeshHandle    ( uint32_t index, uint32_t generation ) const;
    TextureHandle  makeTextureHandle ( uint32_t index, uint32_t generation ) const;
    MaterialHandle makeMaterialHandle( uint32_t index, uint32_t generation ) const;

    uint32_t allocateMeshSlot();
    uint32_t allocateTextureSlot();
    uint32_t allocateMaterialSlot();

    void resetMeshSlot    ( uint32_t index );
    void resetTextureSlot ( uint32_t index );
    void resetMaterialSlot( uint32_t index );
    void releaseMeshSlotByIndex    ( uint32_t index, bool bumpGen );
    void releaseTextureSlotByIndex ( uint32_t index, bool bumpGen );
    void releaseMaterialSlotByIndex( uint32_t index, bool bumpGen );

    const MeshSlot*     tryGetMeshSlot    ( MeshHandle     handle ) const;
    const TextureSlot*  tryGetTextureSlot ( TextureHandle  handle ) const;
    const MaterialSlot* tryGetMaterialSlot( MaterialHandle handle ) const;

    MeshSlot&     requireMeshSlot    ( MeshHandle     handle, const char* callSite );
    TextureSlot&  requireTextureSlot ( TextureHandle  handle, const char* callSite );
    MaterialSlot& requireMaterialSlot( MaterialHandle handle, const char* callSite );

    void validateMaterialTextureHandle( TextureHandle textureHandle, const char* callSite ) const;
    bool isTextureUsedByAnyMaterial   ( TextureHandle textureHandle ) const;

    VulkanDevice& _device;

    std::vector<MeshSlot>     _meshSlots;
    std::vector<uint32_t>     _freeMeshSlots;
    std::unordered_map<std::string, uint32_t> _meshNameToIndex;

    std::vector<TextureSlot>  _textureSlots;
    std::vector<uint32_t>     _freeTextureSlots;
    std::unordered_map<std::string, uint32_t> _textureNameToIndex;

    std::vector<MaterialSlot> _materialSlots;
    std::vector<uint32_t>     _freeMaterialSlots;
    std::unordered_map<std::string, uint32_t> _materialNameToIndex;

    std::function<void()> _textureBindingsChangedCallback;
};
