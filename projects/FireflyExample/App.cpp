#include "App.h"

#include <algorithm>
#include <chrono>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <unordered_map>

namespace
{
    namespace fs = std::filesystem;

    const fs::path kMeshesDir = "./meshes";
    constexpr size_t kFireflyCount = 100;
    constexpr float  kFireflyMinSpeed = 2.5f;
    constexpr float  kFireflyMaxSpeed = 6.5f;
    constexpr float  kFireflyIntensity = 5.0f;
    constexpr float  kFireflyRange = 20.0f;
    constexpr float  kFireflyArrivalEps = 0.6f;
    constexpr float  kFireflyVisualScale = 0.18f;
    const glm::vec3  kFireflyMinBounds = { -40.666721f, 12.0f, -73.554482f };
    const glm::vec3  kFireflyMaxBounds = { 65.1539f, 28.0f, 34.489197f };
    const glm::vec3  kDirtCenter = { 12.243590f, 0.481576f, -19.532642f };
    constexpr float  kInvertedSphereScale = 100.0f;
    constexpr size_t kInvertedSphereLightCount = 300;
    constexpr float  kInvertedSphereLightRadius = 99.9f;
    constexpr float  kInvertedSphereLightIntensity = 3.0f;
    constexpr float  kInvertedSphereLightRange = 40.0f;
    const glm::vec3  kInvertedSphereLightColor = { 1.0f, 0.99f, 0.96f };
    constexpr float  kStarLightIntensity = 1.0f;
    constexpr float  kStarLightRange = 20.0f;
    const glm::vec3  kStarLightColor = { 0.15f, 0.35f, 1.0f };
    const std::array<glm::vec3, 12> kStarLightPositions = { {
        { 1.963735f, 23.166996f, -24.832304f },
        { 15.185807f, 17.894930f, -6.186637f },
        { 15.726083f, 17.999729f, -15.157257f },
        { -16.346747f, 22.348741f, -19.894866f },
        { 2.212150f, 16.990574f, -17.207709f },
        { 20.934364f, 17.318156f, -25.653646f },
        { -27.855212f, 20.040733f, -5.814719f },
        { 28.428498f, 12.464753f, 9.837336f },
        { -3.661417f, 14.259457f, -14.052130f },
        { 34.297385f, 12.182839f, -13.099498f },
        { -6.365817f, 12.210255f, -7.977477f },
        { 9.543082f, 17.583007f, -18.876552f },
    } };

    std::string toLower( std::string value )
    {
        std::transform( value.begin(), value.end(), value.begin(),
                        []( unsigned char c ) {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        return value;
    }

    struct MaterialPreset
    {
        const char* stem;
        const char* name;
        glm::vec4   baseColor;
        float       metallic;
        float       roughness;
    };
    float randomFloat( std::mt19937& rng, float minValue, float maxValue )
    {
        std::uniform_real_distribution<float> distribution( minValue, maxValue );
        return distribution( rng );
    }

    glm::vec3 randomVec3( std::mt19937& rng,
                          const glm::vec3& minValue,
                          const glm::vec3& maxValue )
    {
        return {
            randomFloat( rng, minValue.x, maxValue.x ),
            randomFloat( rng, minValue.y, maxValue.y ),
            randomFloat( rng, minValue.z, maxValue.z ),
        };
    }

    glm::vec3 randomUnitVector( std::mt19937& rng )
    {
        const float z = randomFloat( rng, -1.0f, 1.0f );
        const float angle = randomFloat( rng, 0.0f, 6.28318530717958647692f );
        const float radius = std::sqrt( std::max( 0.0f, 1.0f - z * z ) );

        return {
            radius * std::cos( angle ),
            z,
            radius * std::sin( angle ),
        };
    }

    void spawnRandomSphereLights( Scene& scene,
                                  std::mt19937& rng,
                                  const glm::vec3& center,
                                  float radius,
                                  size_t count,
                                  const glm::vec3& color,
                                  float intensity,
                                  float range )
    {
        for ( size_t i = 0; i < count; ++i )
        {
            scene.createLight(
                LightType::Point,
                center + randomUnitVector( rng ) * radius,
                color,
                intensity,
                range );
        }
    }

    // Convert HSV (h in [0,1), s in [0,1], v in [0,1]) to RGB
    glm::vec3 hsvToRgb( const glm::vec3& hsv )
    {
        float h = hsv.x * 6.0f; // sector 0..5
        float s = hsv.y;
        float v = hsv.z;

        int i = static_cast<int>( std::floor( h ) ) % 6;
        float f = h - std::floor( h );
        float p = v * ( 1.0f - s );
        float q = v * ( 1.0f - f * s );
        float t = v * ( 1.0f - ( 1.0f - f ) * s );

        switch ( i )
        {
            case 0: return glm::vec3( v, t, p );
            case 1: return glm::vec3( q, v, p );
            case 2: return glm::vec3( p, v, t );
            case 3: return glm::vec3( p, q, v );
            case 4: return glm::vec3( t, p, v );
            default: return glm::vec3( v, p, q );
        }
    }

    // Generate a "pure" firefly color: random hue, high saturation and value
    glm::vec3 randomFireflyColor( std::mt19937& rng )
    {
        const float h = randomFloat( rng, 0.0f, 1.0f );
        const float s = randomFloat( rng, 0.6f, 1.0f );
        const float v = randomFloat( rng, 0.6f, 1.0f );
        return hsvToRgb( glm::vec3( h, s, v ) );
    }
}
// ---------------------------------------------------------------------------
// App
// ---------------------------------------------------------------------------

bool App::run()
{
    bool initialized = false;

    try
    {
        _engine.init( "TestApp" );
        initialized = true;

        loadResources();
        populateScene();
        mainLoop();

        safeCleanup( initialized );
        return true;
    }
    catch ( const SceneException& e )
    {
        std::cerr << "Scene error ["
                  << static_cast<int>( e.code() ) << "]: "
                  << e.what() << "\n";
        safeCleanup( initialized );
    }
    catch ( const ResourceException& e )
    {
        std::cerr << "Resource error ["
                  << static_cast<int>( e.code() ) << "]: "
                  << e.what() << "\n";
        safeCleanup( initialized );
    }
    catch ( const std::exception& e )
    {
        std::cerr << "Engine error: " << e.what() << "\n";
        safeCleanup( initialized );
    }
    catch ( ... )
    {
        std::cerr << "Unknown fatal error\n";
        safeCleanup( initialized );
    }

    return false;
}

void App::loadResources()
{
    ResourceManager& resources = _engine.getResourceManager();

    _loadedMeshes.clear();
    _sceneEntities.clear();

    if ( !fs::exists( kMeshesDir ) || !fs::is_directory( kMeshesDir ) )
    {
        throw std::runtime_error( "No existe la carpeta ./meshes para cargar recursos." );
    }

    std::vector<fs::path> objFiles;
    for ( const auto& meshEntry : fs::directory_iterator( kMeshesDir ) )
    {
        if ( !meshEntry.is_regular_file() ) continue;

        const fs::path meshPath = meshEntry.path();
        if ( toLower( meshPath.extension().string() ) != ".obj" ) continue;
        if ( toLower( meshPath.stem().string() ) == "esfera" ) continue;
        if ( toLower( meshPath.stem().string() ) == "esfera_invertida" ) continue;

        objFiles.push_back( meshPath );
    }

    std::sort( objFiles.begin(), objFiles.end() );

    _fireflySphereMesh = resources.createMesh(
        "mesh_fireflySphere",
        ( fs::path( "./meshes" ) / "esfera.obj" ).generic_string() );

    const std::array<MaterialPreset, 7> kMaterialPresets = { {
        { "Bed",    "mat_Bed",    glm::vec4( 0.011f, 0.026f, 0.021f, 0.1f ), 0.00f, 0.88f },
        { "Dirt",   "mat_Dirt",   glm::vec4( 0.02f, 0.15f, 0.15f, 1.0f ), 0.00f, 1.00f },
        { "Grass",  "mat_Grass",  glm::vec4( 0.22f, 0.60f, 0.22f, 1.0f ), 0.00f, 0.92f },
        { "Pillow", "mat_Pillow", glm::vec4( 0.03f, 0.077f, 0.124f, 1.0f ), 0.00f, 0.72f },
        { "Stand",  "mat_Stand",  glm::vec4( 0.112f, 0.314f, 0.521f, 1.0f ), 0.06f, 0.68f },
        { "Stars",  "mat_Stars",  glm::vec4( 0.00f, 0.6f, 0.8f, 1.0f ), 0.18f, 0.22f },
        { "trees",  "mat_trees",  glm::vec4( 0.005f, 0.045f, 0.071f, 1.0f ), 0.00f, 1.00f },
    } };

    std::unordered_map<std::string, MaterialHandle> materialByStem;

    for ( const MaterialPreset& preset : kMaterialPresets )
    {
        MaterialCreateInfo materialInfo;
        materialInfo.baseColor = preset.baseColor;
        materialInfo.metallic = preset.metallic;
        materialInfo.roughness = preset.roughness;

        materialByStem.emplace(
            preset.stem,
            resources.createMaterial( preset.name, materialInfo ) );
    }

    size_t meshesWithMaterial = 0;
    size_t meshesWithoutMaterial = 0;

    for ( const fs::path& objPath : objFiles )
    {
        const std::string stem = objPath.stem().string();
        const std::string objFileName = objPath.filename().string();

        LoadedMeshData loadedMesh;
        loadedMesh.meshName = stem;
        loadedMesh.mesh = resources.createMesh(
            "mesh_" + stem,
            ( fs::path( "./meshes" ) / objFileName ).generic_string() );

        auto matIt = materialByStem.find( stem );
        if ( matIt != materialByStem.end() )
        {
            loadedMesh.material = matIt->second;
            loadedMesh.hasMaterial = true;
            ++meshesWithMaterial;
        }

        if ( !loadedMesh.hasMaterial )
            ++meshesWithoutMaterial;

        _loadedMeshes.push_back( loadedMesh );
    }

    std::cout << "Meshes cargadas: " << _loadedMeshes.size()
              << " | Con material: " << meshesWithMaterial
              << " | Sin material (sin preset manual): "
              << meshesWithoutMaterial << "\n";
}

void App::populateScene()
{
    Scene& scene = _engine.getScene();

    _sceneEntities.clear();
    _fireflies.clear();

    for ( const LoadedMeshData& loadedMesh : _loadedMeshes )
    {
        const MaterialHandle material = loadedMesh.hasMaterial
            ? loadedMesh.material
            : MaterialHandle{};

        _sceneEntities.push_back(
            scene.createEntity( loadedMesh.mesh, material, Transform{} ) );
    }

    MaterialCreateInfo invertedSphereMaterialInfo;
    invertedSphereMaterialInfo.baseColor = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f );
    invertedSphereMaterialInfo.metallic = 0.0f;
    invertedSphereMaterialInfo.roughness = 1.0f;

    const MaterialHandle invertedSphereMaterial = _engine.getResourceManager().createMaterial(
        "mat_InvertedSphere",
        invertedSphereMaterialInfo );

    const MeshHandle invertedSphereMesh = _engine.getResourceManager().createMesh(
        "mesh_invertedSphere",
        ( fs::path( "./meshes" ) / "esfera_invertida.obj" ).generic_string() );

    _sceneEntities.push_back(
        scene.createEntity(
            invertedSphereMesh,
            invertedSphereMaterial,
            Transform{ kDirtCenter, glm::vec3( 0.0f ), glm::vec3( kInvertedSphereScale ) } ) );

    // --- Camara -------------------------------------------------------------
    CameraHandle& camera = scene.getCamera();
    camera.setPosition( { -2.0f, 26.0f, 4.0f } );
    camera.setRotation( { glm::radians( 27.0f ), glm::radians( -135.0f ), 0.0f } );
    camera.setFOV( 90.0f );
    camera.setFarPlane( 1000.0f );

    // --- Iluminacion --------------------------------------------------------

    scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( -1.0f, 1.0f, 0.0f ) ),
        glm::vec3( 0.95f, 0.92f, 0.88f ),
        0.001f
    );
    scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( 1.0f, 1.0f, 0.0f ) ),
        glm::vec3( 0.95f, 0.92f, 0.88f ),
        0.001f
    );
    scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( -1.0f, -1.0f, 0.0f ) ),
        glm::vec3( 0.95f, 0.92f, 0.88f ),
        0.001f
    );
    scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( 1.0f, -1.0f, 0.0f ) ),
        glm::vec3( 0.95f, 0.92f, 0.88f ),
        0.001f
    );
    spawnRandomSphereLights(
        scene,
        _rng,
        kDirtCenter,
        kInvertedSphereLightRadius,
        kInvertedSphereLightCount,
        kInvertedSphereLightColor,
        kInvertedSphereLightIntensity,
        kInvertedSphereLightRange );
    for ( const glm::vec3& position : kStarLightPositions )
    {
        scene.createLight(
            LightType::Point,
            position,
            kStarLightColor,
            kStarLightIntensity,
            kStarLightRange );
    }
    spawnFireflies( scene );
}

void App::mainLoop()
{
    SDL_SetRelativeMouseMode( SDL_TRUE );

    WindowHandler windowHandler( _engine );

    bool  running   = true;
    float deltaTime = 0.0f;

    while ( running )
    {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // --- Eventos --------------------------------------------------------
        SDL_Event ev;
        while ( SDL_PollEvent( &ev ) )
        {
            running = windowHandler.processEvent( ev );
            _cameraController.processEvent( ev );
        }

        // --- Logica ---------------------------------------------------------
        update( deltaTime );

        // --- Render ---------------------------------------------------------
        _engine.drawFrame();

        // --- Delta time -----------------------------------------------------
        auto frameEnd = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(
            frameEnd - frameStart ).count();
    }
}

void App::update( float deltaTime )
{
    CameraHandle& camera = _engine.getScene().getCamera();
    _cameraController.update( camera, deltaTime );
    const glm::vec3 cameraPosition = camera.getPosition();
    const glm::vec3 cameraRotation = camera.getRotation();
    const glm::vec3 cameraRotationDegrees = glm::degrees( cameraRotation );
    updateFireflies( deltaTime );

    (void)deltaTime;
}

void App::spawnFireflies( Scene& scene )
{
    _fireflies.clear();
    _fireflies.reserve( kFireflyCount );

    for ( size_t i = 0; i < kFireflyCount; ++i )
    {
        Firefly firefly;
        firefly.position = randomVec3( _rng, kFireflyMinBounds, kFireflyMaxBounds );
        firefly.target = randomVec3( _rng, kFireflyMinBounds, kFireflyMaxBounds );
        firefly.color = randomFireflyColor( _rng );
        firefly.speed = randomFloat( _rng, kFireflyMinSpeed, kFireflyMaxSpeed );
        firefly.light = scene.createLight(
            LightType::Point,
            firefly.position,
            firefly.color,
            kFireflyIntensity,
            kFireflyRange );

        MaterialCreateInfo fireflyMaterialInfo;
        fireflyMaterialInfo.baseColor = glm::vec4( firefly.color, 1.0f );
        fireflyMaterialInfo.metallic = 0.0f;
        fireflyMaterialInfo.roughness = 0.35f;

        const MaterialHandle fireflyMaterial = _engine.getResourceManager().createMaterial(
            "mat_firefly_" + std::to_string( i ),
            fireflyMaterialInfo );

        firefly.visual = scene.createEntity(
            _fireflySphereMesh,
            fireflyMaterial,
            Transform{ firefly.position, glm::vec3( 0.0f ), glm::vec3( kFireflyVisualScale ) } );

        _fireflies.push_back( std::move( firefly ) );
    }
}

void App::updateFireflies( float deltaTime )
{
    for ( Firefly& firefly : _fireflies )
    {
        glm::vec3 toTarget = firefly.target - firefly.position;
        float distance = glm::length( toTarget );

        if ( distance <= kFireflyArrivalEps )
        {
            firefly.target = randomVec3( _rng, kFireflyMinBounds, kFireflyMaxBounds );
            toTarget = firefly.target - firefly.position;
            distance = glm::length( toTarget );
        }

        if ( distance > 0.0001f )
        {
            const float step = firefly.speed * deltaTime;
            if ( step >= distance )
            {
                firefly.position = firefly.target;
            }
            else
            {
                firefly.position += ( toTarget / distance ) * step;
            }
        }

        firefly.light.setPosOrDir( firefly.position );
        firefly.light.setColor( firefly.color );
        firefly.light.setIntensity( kFireflyIntensity );
        firefly.light.setRange( kFireflyRange );
        firefly.visual.setPosition( firefly.position );
    }
}

void App::freeScene()
{
    _fireflies.clear();
    _engine.getScene().clear();
    _engine.getResourceManager().clear();
}

void App::safeCleanup( bool& initialized ) noexcept
{
    if ( !initialized ) return;

    try
    {
        _engine.wait();
        freeScene();
        _engine.cleanup();
    }
    catch ( ... ) {}

    initialized = false;
}
