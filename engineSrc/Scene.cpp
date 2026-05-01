#include "Scene.h"
#include "RenderEntityHandle.h"
#include "LightEntityHandle.h"

// Inicializa la camara interna y su CameraHandle.
Scene::Scene()
    : _camera( glm::vec3( 0.f, 0.f, -2.5f ),
               glm::vec3( 0.f, 0.f, 0.f ),
               90.f,
               1.f,
               0.1f,
               40.f ),
      _cameraHandle( _camera )
{
}

// ===========================================================================
// Entidades renderizables
// ===========================================================================

RenderEntityHandle Scene::createEntity() {
    const uint32_t index = allocateSlot();
    EntitySlot& slot = _slots[index];
    slot.occupied     = true;
    slot.active       = true;
    slot.visible      = true;
    slot.renderObject = RenderObject{};

    return RenderEntityHandle{ index, getSlotGeneration( index ), this };
}

RenderEntityHandle Scene::createEntity( MeshHandle mesh,
                                         MaterialHandle material,
                                         const Transform& transform ) {
    RenderEntityHandle handle = createEntity();
    handle.setMesh( mesh );
    handle.setMaterial( material );
    handle.setTransform( transform );
    return handle;
}

void Scene::destroyEntity( RenderEntityHandle& handle ) {
    if (handle._scene == nullptr || handle._generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            "Scene::destroyEntity received a handle that was never initialized"
        );
    }
    if (handle._scene != this) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            "Scene::destroyEntity received a handle from a different Scene"
        );
    }

    EntitySlot& slot = requireSlot( handle._index, handle._generation, "Scene::destroyEntity" );
    slot.renderObject = RenderObject{};
    slot.occupied     = false;
    bumpGeneration( slot.generation );
    _freeSlots.push_back( handle._index );

    handle.invalidate();
}

// ===========================================================================
// Luces
// ===========================================================================

LightEntityHandle Scene::createLight( LightType        type,
                                       const glm::vec3& posOrDir,
                                       const glm::vec3& color,
                                       float            intensity,
                                       float            range ) {
    const uint32_t index = allocateLightSlot();
    LightSlot& slot = _lightSlots[index];
    slot.occupied        = true;
    slot.active          = true;
    slot.light.type      = type;
    slot.light.posOrDir  = posOrDir;
    slot.light.color     = color;
    slot.light.intensity = intensity;
    slot.light.range     = range;

    return LightEntityHandle{ index, getLightSlotGeneration( index ), this };
}

void Scene::destroyLight( LightEntityHandle& handle ) {
    if (handle._scene == nullptr || handle._generation == 0) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            "Scene::destroyLight received a handle that was never initialized"
        );
    }
    if (handle._scene != this) {
        throw SceneException(
            SceneErrorCode::InvalidHandle,
            "Scene::destroyLight received a handle from a different Scene"
        );
    }

    LightSlot& slot = requireLightSlot( handle._index, handle._generation, "Scene::destroyLight" );
    slot.light    = LightObject{};
    slot.occupied = false;
    bumpGeneration( slot.generation );
    _freeLightSlots.push_back( handle._index );

    handle.invalidate();
}

// ---------------------------------------------------------------------------
// Main light (shadow caster)
// ---------------------------------------------------------------------------

void Scene::setMainLight(const LightEntityHandle& handle) {
    // Validacion de handle: lanza InvalidHandle o StaleHandle si no es valido.
    const LightSlot& slot = requireLightSlotConst(
        handle._index, handle._generation, "Scene::setMainLight");

    if (slot.light.type != LightType::Directional) {
        throw std::invalid_argument(
            "Scene::setMainLight: only Directional lights can be the main light (shadow caster)");
    }

    _mainLightRef = MainLightRef{ handle._index, handle._generation };
}

void Scene::clearMainLight() {
    _mainLightRef.reset();
}

bool Scene::hasMainLight() const {
    return tryGetMainLight() != nullptr;
}


// ===========================================================================
// Operaciones globales
// ===========================================================================

void Scene::clear() {
    _slots.clear();
    _freeSlots.clear();
    _lightSlots.clear();
    _freeLightSlots.clear();
    _mainLightRef.reset();
}

// ===========================================================================
// Consultas generales
// ===========================================================================

bool Scene::hasEntity( const RenderEntityHandle& handle ) const {
    if (handle._scene != this || handle._generation == 0) return false;
    return hasEntity( handle._index, handle._generation );
}

std::size_t Scene::entityCount() const {
    return _slots.size() - _freeSlots.size();
}

bool Scene::hasLight( const LightEntityHandle& handle ) const {
    if (handle._scene != this || handle._generation == 0) return false;
    return hasLight( handle._index, handle._generation );
}

std::size_t Scene::lightCount() const {
    return _lightSlots.size() - _freeLightSlots.size();
}

// ===========================================================================
// Acceso interno a slots de entidades (RenderEntityHandle)
// ===========================================================================

Scene::EntitySlot& Scene::requireSlot( uint32_t index, uint32_t generation, const char* callSite ) {
    EntitySlot* slot = tryGetSlot( index, generation );
    if (slot == nullptr) {
        if (!isSlotIndexInRange( index )) {
            throw SceneException( SceneErrorCode::InvalidHandle,
                std::string( callSite ) + ": handle index is out of range" );
        }
        throw SceneException( SceneErrorCode::StaleHandle,
            std::string( callSite ) + ": entity has already been destroyed" );
    }
    return *slot;
}

const Scene::EntitySlot& Scene::requireSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const {
    const EntitySlot* slot = tryGetSlot( index, generation );
    if (slot == nullptr) {
        if (!isSlotIndexInRange( index )) {
            throw SceneException( SceneErrorCode::InvalidHandle,
                std::string( callSite ) + ": handle index is out of range" );
        }
        throw SceneException( SceneErrorCode::StaleHandle,
            std::string( callSite ) + ": entity has already been destroyed" );
    }
    return *slot;
}

// ===========================================================================
// Acceso interno a slots de luces (LightEntityHandle)
// ===========================================================================

Scene::LightSlot& Scene::requireLightSlot( uint32_t index, uint32_t generation, const char* callSite ) {
    LightSlot* slot = tryGetLightSlot( index, generation );
    if (slot == nullptr) {
        if (!isLightIndexInRange( index )) {
            throw SceneException( SceneErrorCode::InvalidHandle,
                std::string( callSite ) + ": light handle index is out of range" );
        }
        throw SceneException( SceneErrorCode::StaleHandle,
            std::string( callSite ) + ": light has already been destroyed" );
    }
    return *slot;
}

const Scene::LightSlot& Scene::requireLightSlotConst( uint32_t index, uint32_t generation, const char* callSite ) const {
    const LightSlot* slot = tryGetLightSlot( index, generation );
    if (slot == nullptr) {
        if (!isLightIndexInRange( index )) {
            throw SceneException( SceneErrorCode::InvalidHandle,
                std::string( callSite ) + ": light handle index is out of range" );
        }
        throw SceneException( SceneErrorCode::StaleHandle,
            std::string( callSite ) + ": light has already been destroyed" );
    }
    return *slot;
}

// ===========================================================================
// Render queue, light queue y main light (RenderEngine)
// ===========================================================================

const std::vector<RenderObject>& Scene::buildRenderQueue() const {
    _renderQueueCache.clear();
    _renderQueueCache.reserve( entityCount() );

    for (const EntitySlot& slot : _slots) {
        if (!slot.occupied || !slot.active || !slot.visible) continue;
        if (!slot.renderObject.mesh.isValid()) continue;
        _renderQueueCache.push_back( slot.renderObject );
    }

    return _renderQueueCache;
}

const std::vector<LightObject>& Scene::buildLightQueue() const {
    _lightQueueCache.clear();
    _lightQueueCache.reserve( lightCount() );

    for (const LightSlot& slot : _lightSlots) {
        if (!slot.occupied || !slot.active) continue;
        _lightQueueCache.push_back( slot.light );
    }

    return _lightQueueCache;
}

const LightObject* Scene::tryGetMainLight() const {
    if (!_mainLightRef.has_value()) {
        return nullptr;
    }

    // Si la luz fue destruida entre frames, su generacion en el slot
    // no coincidira y tryGetLightSlot devolvera nullptr.
    const LightSlot* slot = tryGetLightSlot(
        _mainLightRef->index, _mainLightRef->generation);

    if (slot == nullptr || !slot->active) {
        return nullptr;
    }

    return &slot->light;
}

// ===========================================================================
// Implementacion interna
// ===========================================================================

bool Scene::hasEntity( uint32_t index, uint32_t generation ) const {
    return tryGetSlot( index, generation ) != nullptr;
}

bool Scene::hasLight( uint32_t index, uint32_t generation ) const {
    return tryGetLightSlot( index, generation ) != nullptr;
}

Scene::EntitySlot* Scene::tryGetSlot( uint32_t index, uint32_t generation ) {
    if (index >= _slots.size()) return nullptr;
    EntitySlot& slot = _slots[index];
    if (!slot.occupied || slot.generation != generation) return nullptr;
    return &slot;
}

const Scene::EntitySlot* Scene::tryGetSlot( uint32_t index, uint32_t generation ) const {
    if (index >= _slots.size()) return nullptr;
    const EntitySlot& slot = _slots[index];
    if (!slot.occupied || slot.generation != generation) return nullptr;
    return &slot;
}

Scene::LightSlot* Scene::tryGetLightSlot( uint32_t index, uint32_t generation ) {
    if (index >= _lightSlots.size()) return nullptr;
    LightSlot& slot = _lightSlots[index];
    if (!slot.occupied || slot.generation != generation) return nullptr;
    return &slot;
}

const Scene::LightSlot* Scene::tryGetLightSlot( uint32_t index, uint32_t generation ) const {
    if (index >= _lightSlots.size()) return nullptr;
    const LightSlot& slot = _lightSlots[index];
    if (!slot.occupied || slot.generation != generation) return nullptr;
    return &slot;
}

bool Scene::isSlotIndexInRange( uint32_t index ) const {
    return index < _slots.size();
}

bool Scene::isLightIndexInRange( uint32_t index ) const {
    return index < _lightSlots.size();
}

uint32_t Scene::getSlotGeneration( uint32_t index ) const {
    return _slots[index].generation;
}

uint32_t Scene::getLightSlotGeneration( uint32_t index ) const {
    return _lightSlots[index].generation;
}

uint32_t Scene::allocateSlot() {
    if (!_freeSlots.empty()) {
        const uint32_t index = _freeSlots.back();
        _freeSlots.pop_back();
        return index;
    }
    if (_slots.size() >= MAX_ENTITIES) {
        throw SceneException( SceneErrorCode::LimitExceeded,
            "Scene::createEntity exceeded MAX_ENTITIES (" + std::to_string( MAX_ENTITIES ) + ")" );
    }
    _slots.emplace_back();
    return static_cast<uint32_t>(_slots.size() - 1);
}

uint32_t Scene::allocateLightSlot() {
    if (!_freeLightSlots.empty()) {
        const uint32_t index = _freeLightSlots.back();
        _freeLightSlots.pop_back();
        return index;
    }
    if (_lightSlots.size() >= MAX_LIGHTS) {
        throw SceneException( SceneErrorCode::LimitExceeded,
            "Scene::createLight exceeded MAX_LIGHTS (" + std::to_string( MAX_LIGHTS ) + ")" );
    }
    _lightSlots.emplace_back();
    return static_cast<uint32_t>(_lightSlots.size() - 1);
}

void Scene::bumpGeneration( uint32_t& generation ) {
    ++generation;
    if (generation == 0) generation = 1;
}
