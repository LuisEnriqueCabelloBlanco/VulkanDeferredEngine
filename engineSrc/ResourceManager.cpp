#include "ResourceManager.h"

#include <utility>

ResourceManager::ResourceManager( VulkanDevice& device )
    : _device( device ) {
}

void ResourceManager::setTextureBindingsChangedCallback( std::function<void()> callback ) {
    _textureBindingsChangedCallback = std::move( callback );
}

void ResourceManager::clear() {
    // Liberamos todos los recursos en orden de dependencias: materiales primero (rompen refs a texturas),
    // texturas después, meshes al final.
    releaseAllMaterials();
    releaseAllTextures();
    releaseAllMeshes();

    // Luego limpiamos todos los maps y free-lists.
    _meshSlots.clear();
    _freeMeshSlots.clear();
    _meshNameToIndex.clear();

    _materialSlots.clear();
    _freeMaterialSlots.clear();
    _materialNameToIndex.clear();

    _textureSlots.clear();
    _freeTextureSlots.clear();
    _textureNameToIndex.clear();
}

MeshHandle ResourceManager::createMesh( const std::string& name, const std::string& path ) {
    validateResourceName( name, "ResourceManager::createMesh" );
    if (path.empty()) {
        throw ResourceException( ResourceErrorCode::LoadFailed, "ResourceManager::createMesh requires a non-empty path" );
    }

    // El nombre es la clave publica: nunca se permiten dos recursos con el mismo nombre.
    if (_meshNameToIndex.find( name ) != _meshNameToIndex.end()) {
        throw ResourceException( ResourceErrorCode::DuplicateName, makeDuplicateNameMessage( "ResourceManager::createMesh", name ) );
    }

    const uint32_t index = allocateMeshSlot();
    MeshSlot& slot = _meshSlots[index];

    try {
        // La construccion puede lanzar por IO/parseo/Vulkan, por eso se hace con rollback.
        slot.resource = std::make_unique<Mesh>( _device, path );
    }
    catch (const std::exception& e) {
        resetMeshSlot( index );
        throw ResourceException( ResourceErrorCode::LoadFailed, std::string( "ResourceManager::createMesh failed for '" ) + name + "': " + e.what() );
    }

    slot.occupied = true;
    slot.name = name;
    _meshNameToIndex[name] = index;

    return makeMeshHandle( index, slot.generation );
}

MeshHandle ResourceManager::createMesh( const std::string& name, const std::vector<Vertex>& vertices ) {
    validateResourceName( name, "ResourceManager::createMesh" );

    if (_meshNameToIndex.find( name ) != _meshNameToIndex.end()) {
        throw ResourceException( ResourceErrorCode::DuplicateName, makeDuplicateNameMessage( "ResourceManager::createMesh", name ) );
    }

    const uint32_t index = allocateMeshSlot();
    MeshSlot& slot = _meshSlots[index];

    try {
        slot.resource = std::make_unique<Mesh>( _device, vertices );
    }
    catch (const std::exception& e) {
        resetMeshSlot( index );
        throw ResourceException( ResourceErrorCode::LoadFailed, std::string( "ResourceManager::createMesh failed for '" ) + name + "': " + e.what() );
    }

    slot.occupied = true;
    slot.name = name;
    _meshNameToIndex[name] = index;

    return makeMeshHandle( index, slot.generation );
}

MeshHandle ResourceManager::createMesh( const std::string& name, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices ) {
    validateResourceName( name, "ResourceManager::createMesh" );

    if (_meshNameToIndex.find( name ) != _meshNameToIndex.end()) {
        throw ResourceException( ResourceErrorCode::DuplicateName, makeDuplicateNameMessage( "ResourceManager::createMesh", name ) );
    }

    const uint32_t index = allocateMeshSlot();
    MeshSlot& slot = _meshSlots[index];

    try {
        slot.resource = std::make_unique<Mesh>( _device, indices, vertices );
    }
    catch (const std::exception& e) {
        resetMeshSlot( index );
        throw ResourceException( ResourceErrorCode::LoadFailed, std::string( "ResourceManager::createMesh failed for '" ) + name + "': " + e.what() );
    }

    slot.occupied = true;
    slot.name = name;
    _meshNameToIndex[name] = index;

    return makeMeshHandle( index, slot.generation );
}

void ResourceManager::releaseMesh( MeshHandle handle ) {
    // Valida generation e index antes de tocar el slot real.
    requireMeshSlot( handle, "ResourceManager::releaseMesh" );
    releaseMeshSlotByIndex( handle._index, true );
}

void ResourceManager::releaseAllMeshes() {
    for (uint32_t index = 0; index < _meshSlots.size(); ++index) {
        MeshSlot& slot = _meshSlots[index];
        if (!slot.occupied) {
            continue;
        }

        releaseMeshSlotByIndex( index, true );
    }
}

TextureHandle ResourceManager::createTexture( const std::string& name, const std::string& path ) {
    validateResourceName( name, "ResourceManager::createTexture" );
    if (path.empty()) {
        throw ResourceException( ResourceErrorCode::LoadFailed, "ResourceManager::createTexture requires a non-empty path" );
    }

    // Igual que en meshes: el nombre define unicidad, no el contenido del fichero.
    if (_textureNameToIndex.find( name ) != _textureNameToIndex.end()) {
        throw ResourceException( ResourceErrorCode::DuplicateName, makeDuplicateNameMessage( "ResourceManager::createTexture", name ) );
    }

    const uint32_t index = allocateTextureSlot();
    TextureSlot& slot = _textureSlots[index];

    try {
        // Texture separa construccion GPU y carga de archivo; ambos pasos pueden fallar.
        slot.resource = std::make_unique<Texture>( _device );
        slot.resource->loadTexture( path );
    }
    catch (const std::exception& e) {
        resetTextureSlot( index );
        throw ResourceException( ResourceErrorCode::LoadFailed, std::string( "ResourceManager::createTexture failed for '" ) + name + "': " + e.what() );
    }

    slot.occupied = true;
    slot.name = name;
    _textureNameToIndex[name] = index;

    notifyTextureBindingsChanged();

    return makeTextureHandle( index, slot.generation );
}

void ResourceManager::releaseTexture( TextureHandle handle ) {
    requireTextureSlot( handle, "ResourceManager::releaseTexture" );

    /// Valida que no hay materiales que dependan de esta textura
    if (isTextureUsedByAnyMaterial( handle )) {
        throw ResourceException(
            ResourceErrorCode::DependencyInUse,
            "ResourceManager::releaseTexture rejected: texture is referenced by at least one live material; "
            "release dependent materials first (releaseMaterial/releaseAllMaterials)"
        );
    }

    releaseTextureSlotByIndex( handle._index, true );

    notifyTextureBindingsChanged();
}

void ResourceManager::releaseAllTextures() {
    for (uint32_t index = 0; index < _textureSlots.size(); ++index) {
        TextureSlot& slot = _textureSlots[index];
        if (!slot.occupied) {
            continue;
        }

        // Valida que no hay materiales que dependan de esta textura
        TextureHandle handle = makeTextureHandle( index, slot.generation );
        if (isTextureUsedByAnyMaterial( handle )) {
            throw ResourceException(
                ResourceErrorCode::DependencyInUse,
                "ResourceManager::releaseAllTextures detected a live material still referencing a texture; "
                "call releaseAllMaterials() before releaseAllTextures()"
            );
        }

        releaseTextureSlotByIndex( index, true );
    }

    notifyTextureBindingsChanged();
}

MaterialHandle ResourceManager::createMaterial( const std::string& name, const MaterialCreateInfo& material ) {
    validateResourceName( name, "ResourceManager::createMaterial" );

    // Materiales tambien se nombran de forma explicita para mantener el contrato simple.
    if (_materialNameToIndex.find( name ) != _materialNameToIndex.end()) {
        throw ResourceException( ResourceErrorCode::DuplicateName, makeDuplicateNameMessage( "ResourceManager::createMaterial", name ) );
    }

    const uint32_t index = allocateMaterialSlot();
    MaterialSlot& slot = _materialSlots[index];

    try {
        validateMaterialTextureHandle( material.baseColorTexture, "ResourceManager::createMaterial" );
        validateMaterialTextureHandle( material.normalTexture, "ResourceManager::createMaterial" );

        slot.data.baseColor = material.baseColor;
        slot.data.metallic = material.metallic;
        slot.data.roughness = material.roughness;
        slot.data.textureIndex = material.baseColorTexture.isValid() ? static_cast<int>( material.baseColorTexture._index ) : -1;
        slot.data.normalTextureIndex = material.normalTexture.isValid() ? static_cast<int>( material.normalTexture._index ) : -1;
        slot.name = name;
        slot.occupied = true;
        _materialNameToIndex[name] = index;
    }
    catch (const std::exception& e) {
        // Rollback: slot se vuelve a un estado limpio si la validacion falla.
        resetMaterialSlot( index );
        throw ResourceException( ResourceErrorCode::LoadFailed, std::string( "ResourceManager::createMaterial failed for '" ) + name + "': " + e.what() );
    }

    return makeMaterialHandle( index, slot.generation );
}

void ResourceManager::resetMaterialSlot( uint32_t index ) {
    releaseMaterialSlotByIndex( index, false );
}

void ResourceManager::releaseMaterial( MaterialHandle handle ) {
    requireMaterialSlot( handle, "ResourceManager::releaseMaterial" );
    releaseMaterialSlotByIndex( handle._index, true );
}

void ResourceManager::releaseAllMaterials() {
    for (uint32_t index = 0; index < _materialSlots.size(); ++index) {
        MaterialSlot& slot = _materialSlots[index];
        if (!slot.occupied) {
            continue;
        }

        releaseMaterialSlotByIndex( index, true );
    }
}

MeshHandle ResourceManager::tryGetMeshHandle( const std::string& name ) const {
    // Devuelve un handle vivo solo si el nombre sigue apuntando a un slot ocupado.
    auto it = _meshNameToIndex.find( name );
    if (it == _meshNameToIndex.end()) {
        return MeshHandle{};
    }

    const uint32_t index = it->second;
    if (index >= _meshSlots.size()) {
        return MeshHandle{};
    }

    const MeshSlot& slot = _meshSlots[index];
    if (!slot.occupied) {
        return MeshHandle{};
    }

    return makeMeshHandle( index, slot.generation );
}

TextureHandle ResourceManager::tryGetTextureHandle( const std::string& name ) const {
    // Se usa para reconstruir flujos que parten de nombres, no de handles previos.
    auto it = _textureNameToIndex.find( name );
    if (it == _textureNameToIndex.end()) {
        return TextureHandle{};
    }

    const uint32_t index = it->second;
    if (index >= _textureSlots.size()) {
        return TextureHandle{};
    }

    const TextureSlot& slot = _textureSlots[index];
    if (!slot.occupied) {
        return TextureHandle{};
    }

    return makeTextureHandle( index, slot.generation );
}

MaterialHandle ResourceManager::tryGetMaterialHandle( const std::string& name ) const {
    // Mantiene la misma semantica que meshes y textures: nombre -> handle tecnico.
    auto it = _materialNameToIndex.find( name );
    if (it == _materialNameToIndex.end()) {
        return MaterialHandle{};
    }

    const uint32_t index = it->second;
    if (index >= _materialSlots.size()) {
        return MaterialHandle{};
    }

    const MaterialSlot& slot = _materialSlots[index];
    if (!slot.occupied) {
        return MaterialHandle{};
    }

    return makeMaterialHandle( index, slot.generation );
}

const Mesh* ResourceManager::tryGetMesh( MeshHandle handle ) const {
    const MeshSlot* slot = tryGetMeshSlot( handle );
    return (slot == nullptr) ? nullptr : slot->resource.get();
}

const Texture* ResourceManager::tryGetTexture( TextureHandle handle ) const {
    const TextureSlot* slot = tryGetTextureSlot( handle );
    return (slot == nullptr) ? nullptr : slot->resource.get();
}

const MaterialData* ResourceManager::tryGetMaterial( MaterialHandle handle ) const {
    const MaterialSlot* slot = tryGetMaterialSlot( handle );
    return (slot == nullptr) ? nullptr : &slot->data;
}

std::vector<ResourceManager::TextureBindingEntry> ResourceManager::getLiveTextureEntries() const {
    // El renderer necesita el indice real del slot para escribir el descriptor array.
    std::vector<TextureBindingEntry> out;
    out.reserve( _textureNameToIndex.size() );

    for (uint32_t i = 0; i < _textureSlots.size(); ++i) {
        const TextureSlot& slot = _textureSlots[i];
        if (!slot.occupied || slot.resource == nullptr) {
            continue;
        }
        out.push_back( TextureBindingEntry{ i, slot.resource.get() } );
    }

    return out;
}

void ResourceManager::notifyTextureBindingsChanged() const {
    if (_textureBindingsChangedCallback) {
        _textureBindingsChangedCallback();
    }
}

void ResourceManager::bumpGeneration( uint32_t& generation ) {
    ++generation;
    if (generation == 0) {
        generation = 1;
    }
}

void ResourceManager::validateResourceName( const std::string& name, const char* callSite ) {
    if (name.empty()) {
        throw ResourceException( ResourceErrorCode::InvalidName, std::string( callSite ) + " requires a non-empty resource name" );
    }
}

std::string ResourceManager::makeDuplicateNameMessage( const char* callSite, const std::string& name ) {
    return std::string( callSite ) + " duplicated name: " + name;
}

MeshHandle ResourceManager::makeMeshHandle( uint32_t index, uint32_t generation ) const {
    return MeshHandle( index, generation );
}

TextureHandle ResourceManager::makeTextureHandle( uint32_t index, uint32_t generation ) const {
    return TextureHandle( index, generation );
}

MaterialHandle ResourceManager::makeMaterialHandle( uint32_t index, uint32_t generation ) const {
    return MaterialHandle( index, generation );
}

uint32_t ResourceManager::allocateMeshSlot() {
    // Reutiliza huecos libres antes de crecer la tabla.
    if (!_freeMeshSlots.empty()) {
        const uint32_t index = _freeMeshSlots.back();
        _freeMeshSlots.pop_back();
        return index;
    }

    if (_meshSlots.size() >= static_cast<size_t>(ResourceLimits::MAX_MESHES)) {
        throw ResourceException( ResourceErrorCode::LimitExceeded, "ResourceManager::createMesh exceeded MAX_MESHES" );
    }

    _meshSlots.emplace_back();
    return static_cast<uint32_t>(_meshSlots.size() - 1);
}

uint32_t ResourceManager::allocateTextureSlot() {
    // Las texturas ocupan espacio de descriptor fijo, asi que el limite es rigido.
    if (!_freeTextureSlots.empty()) {
        const uint32_t index = _freeTextureSlots.back();
        _freeTextureSlots.pop_back();
        return index;
    }

    if (_textureSlots.size() >= static_cast<size_t>(ResourceLimits::MAX_TEXTURES)) {
        throw ResourceException( ResourceErrorCode::LimitExceeded, "ResourceManager::createTexture exceeded MAX_TEXTURES" );
    }

    _textureSlots.emplace_back();
    return static_cast<uint32_t>(_textureSlots.size() - 1);
}

uint32_t ResourceManager::allocateMaterialSlot() {
    // Los materiales viven en CPU, pero siguen el mismo patron de slots y reutilizacion.
    if (!_freeMaterialSlots.empty()) {
        const uint32_t index = _freeMaterialSlots.back();
        _freeMaterialSlots.pop_back();
        return index;
    }

    if (_materialSlots.size() >= static_cast<size_t>(ResourceLimits::MAX_MATERIALS)) {
        throw ResourceException( ResourceErrorCode::LimitExceeded, "ResourceManager::createMaterial exceeded MAX_MATERIALS" );
    }

    _materialSlots.emplace_back();
    return static_cast<uint32_t>(_materialSlots.size() - 1);
}

void ResourceManager::resetMeshSlot( uint32_t index ) {
    releaseMeshSlotByIndex( index, false );
}

void ResourceManager::resetTextureSlot( uint32_t index ) {
    releaseTextureSlotByIndex( index, false );
}

void ResourceManager::releaseMeshSlotByIndex( uint32_t index, bool bumpGen ) {
    MeshSlot& slot = _meshSlots[index];
    _meshNameToIndex.erase( slot.name );
    slot.resource.reset();
    slot.occupied = false;
    slot.name.clear();
    if (bumpGen) {
        bumpGeneration( slot.generation );
    }
    _freeMeshSlots.push_back( index );
}

void ResourceManager::releaseTextureSlotByIndex( uint32_t index, bool bumpGen ) {
    TextureSlot& slot = _textureSlots[index];
    _textureNameToIndex.erase( slot.name );
    slot.resource.reset();
    slot.occupied = false;
    slot.name.clear();
    if (bumpGen) {
        bumpGeneration( slot.generation );
    }
    _freeTextureSlots.push_back( index );
}

void ResourceManager::releaseMaterialSlotByIndex( uint32_t index, bool bumpGen ) {
    MaterialSlot& slot = _materialSlots[index];
    _materialNameToIndex.erase( slot.name );
    slot.occupied = false;
    slot.name.clear();
    slot.data = MaterialData{};
    if (bumpGen) {
        bumpGeneration( slot.generation );
    }
    _freeMaterialSlots.push_back( index );
}

const ResourceManager::MeshSlot* ResourceManager::tryGetMeshSlot( MeshHandle handle ) const {
    if (!handle.isValid() || handle._index >= _meshSlots.size()) {
        return nullptr;
    }

    const MeshSlot& slot = _meshSlots[handle._index];
    if (!slot.occupied || slot.generation != handle._generation) {
        return nullptr;
    }

    return &slot;
}

const ResourceManager::TextureSlot* ResourceManager::tryGetTextureSlot( TextureHandle handle ) const {
    if (!handle.isValid() || handle._index >= _textureSlots.size()) {
        return nullptr;
    }

    const TextureSlot& slot = _textureSlots[handle._index];
    if (!slot.occupied || slot.generation != handle._generation) {
        return nullptr;
    }

    return &slot;
}

const ResourceManager::MaterialSlot* ResourceManager::tryGetMaterialSlot( MaterialHandle handle ) const {
    if (!handle.isValid() || handle._index >= _materialSlots.size()) {
        return nullptr;
    }

    const MaterialSlot& slot = _materialSlots[handle._index];
    if (!slot.occupied || slot.generation != handle._generation) {
        return nullptr;
    }

    return &slot;
}

ResourceManager::MeshSlot& ResourceManager::requireMeshSlot( MeshHandle handle, const char* callSite ) {
    if (!handle.isValid() || handle._index >= _meshSlots.size()) {
        throw ResourceException( ResourceErrorCode::InvalidHandle, std::string( callSite ) + " received invalid mesh handle" );
    }

    MeshSlot& slot = _meshSlots[handle._index];
    if (!slot.occupied || slot.generation != handle._generation) {
        throw ResourceException( ResourceErrorCode::StaleHandle, std::string( callSite ) + " received stale mesh handle" );
    }

    return slot;
}

ResourceManager::TextureSlot& ResourceManager::requireTextureSlot( TextureHandle handle, const char* callSite ) {
    if (!handle.isValid() || handle._index >= _textureSlots.size()) {
        throw ResourceException( ResourceErrorCode::InvalidHandle, std::string( callSite ) + " received invalid texture handle" );
    }

    TextureSlot& slot = _textureSlots[handle._index];
    if (!slot.occupied || slot.generation != handle._generation) {
        throw ResourceException( ResourceErrorCode::StaleHandle, std::string( callSite ) + " received stale texture handle" );
    }

    return slot;
}

ResourceManager::MaterialSlot& ResourceManager::requireMaterialSlot( MaterialHandle handle, const char* callSite ) {
    if (!handle.isValid() || handle._index >= _materialSlots.size()) {
        throw ResourceException( ResourceErrorCode::InvalidHandle, std::string( callSite ) + " received invalid material handle" );
    }

    MaterialSlot& slot = _materialSlots[handle._index];
    if (!slot.occupied || slot.generation != handle._generation) {
        throw ResourceException( ResourceErrorCode::StaleHandle, std::string( callSite ) + " received stale material handle" );
    }

    return slot;
}

void ResourceManager::validateMaterialTextureHandle( TextureHandle textureHandle, const char* callSite ) const {
    if (!textureHandle.isValid()) {
        return;
    }

    if (textureHandle._index >= static_cast<uint32_t>(ResourceLimits::MAX_TEXTURES)) {
        throw ResourceException(
            ResourceErrorCode::LimitExceeded,
            std::string( callSite ) + " received a texture handle outside MAX_TEXTURES"
        );
    }

    if (tryGetTextureSlot( textureHandle ) == nullptr) {
        throw ResourceException(
            ResourceErrorCode::InvalidHandle,
            std::string( callSite ) + " received an invalid or stale texture handle"
        );
    }
}

bool ResourceManager::isTextureUsedByAnyMaterial( TextureHandle textureHandle ) const {
    for (const auto& materialSlot : _materialSlots) {
        if (!materialSlot.occupied) {
            continue;
        }

        const bool usedByBaseColor = materialSlot.data.textureIndex == static_cast<int>(textureHandle._index);
        const bool usedByNormal = materialSlot.data.normalTextureIndex == static_cast<int>(textureHandle._index);

        if (usedByBaseColor || usedByNormal) {
            return true;
        }
    }

    return false;
}
