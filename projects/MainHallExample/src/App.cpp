#include "App.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace
{
    namespace fs = std::filesystem;

    const fs::path kMeshesDir = "./assets/meshes";
    const fs::path kTexturesDir = "./assets/textures";

    bool startsWith( const std::string& value, const std::string& prefix )
    {
        return value.size() >= prefix.size() &&
               value.compare( 0, prefix.size(), prefix ) == 0;
    }

    std::string trim( const std::string& value )
    {
        const size_t first = value.find_first_not_of( " \t\r\n" );
        if ( first == std::string::npos ) return "";

        const size_t last = value.find_last_not_of( " \t\r\n" );
        return value.substr( first, last - first + 1 );
    }

    std::string toLower( std::string value )
    {
        std::transform( value.begin(), value.end(), value.begin(),
                        []( unsigned char c ) {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        return value;
    }

    std::string makeSafeName( const std::string& value )
    {
        std::string safe = value;
        std::transform( safe.begin(), safe.end(), safe.begin(),
                        []( unsigned char c ) {
                            if ( std::isalnum( c ) ) return static_cast<char>( c );
                            return '_';
                        } );
        return safe;
    }

    std::optional<std::string> tryGetTextureFromMtl( const fs::path& mtlPath )
    {
        std::ifstream file( mtlPath );
        if ( !file.is_open() ) return std::nullopt;

        std::string line;
        while ( std::getline( file, line ) )
        {
            const size_t commentPos = line.find( '#' );
            if ( commentPos != std::string::npos )
                line = line.substr( 0, commentPos );

            const std::string cleaned = trim( line );
            if ( !startsWith( cleaned, "map_Kd" ) ) continue;

            std::string rest = trim( cleaned.substr( 6 ) );
            if ( rest.empty() ) continue;

            std::istringstream iss( rest );
            std::vector<std::string> tokens;
            std::string token;
            while ( iss >> token )
                tokens.push_back( token );

            if ( tokens.empty() ) continue;

            const std::string fileName = fs::path( tokens.back() ).filename().string();
            if ( !fileName.empty() )
                return fileName;
        }

        return std::nullopt;
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

    if ( !fs::exists( kTexturesDir ) || !fs::is_directory( kTexturesDir ) )
    {
        throw std::runtime_error( "No existe la carpeta ./textures para cargar recursos." );
    }

    _lightMarkerMesh = resources.createMesh( "mesh_light_marker_cube", "./assets/meshes/cubo.obj" );

    MaterialCreateInfo markerMatInfo;
    markerMatInfo.baseColor = glm::vec4( 1.0f, 0.95f, 0.6f, 1.0f );
    markerMatInfo.metallic = 0.0f;
    markerMatInfo.roughness = 0.25f;
    _lightMarkerMat = resources.createMaterial( "mat_light_marker_cube", markerMatInfo );

    _floorMesh = resources.createMesh( "mesh_floor_plane", "./assets/meshes/plano.obj" );

    std::unordered_map<std::string, std::string> textureFileByExactName;
    std::unordered_map<std::string, std::string> textureFileByStem;
    for ( const auto& textureEntry : fs::directory_iterator( kTexturesDir ) )
    {
        if ( !textureEntry.is_regular_file() ) continue;

        const std::string textureFile = textureEntry.path().filename().string();
        const std::string textureKey = toLower( textureFile );
        textureFileByExactName.emplace( textureKey, textureFile );

        const std::string textureStem = toLower( textureEntry.path().stem().string() );
        textureFileByStem.emplace( textureStem, textureFile );
    }

    std::vector<fs::path> objFiles;
    for ( const auto& meshEntry : fs::directory_iterator( kMeshesDir ) )
    {
        if ( !meshEntry.is_regular_file() ) continue;

        const fs::path meshPath = meshEntry.path();
        const std::string fileName = meshPath.filename().string();
        if ( !startsWith( fileName, "Mesh_" ) ) continue;
        if ( toLower( meshPath.extension().string() ) != ".obj" ) continue;

        objFiles.push_back( meshPath );
    }

    std::sort( objFiles.begin(), objFiles.end() );

    auto resolveTextureFile = [&]( const std::string& requestedFile ) -> std::string {
        const std::string requestedKey = toLower( requestedFile );

        auto exactTextureIt = textureFileByExactName.find( requestedKey );
        if ( exactTextureIt != textureFileByExactName.end() )
            return exactTextureIt->second;

        const std::string requestedStem = toLower( fs::path( requestedFile ).stem().string() );
        auto stemTextureIt = textureFileByStem.find( requestedStem );
        if ( stemTextureIt != textureFileByStem.end() )
            return stemTextureIt->second;

        return {};
    };

    std::unordered_map<std::string, TextureHandle> textureByFile;
    std::unordered_map<std::string, MaterialHandle> materialByFile;

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
            ( fs::path( "./assets/meshes" ) / objFileName ).generic_string() );

        const fs::path mtlPath = kMeshesDir / ( stem + ".mtl" );
        if ( fs::exists( mtlPath ) && fs::is_regular_file( mtlPath ) )
        {
            const std::optional<std::string> mtlTexture = tryGetTextureFromMtl( mtlPath );
            if ( mtlTexture.has_value() )
            {
                const std::string textureFile = mtlTexture.value();
                const std::string resolvedTextureFile = resolveTextureFile( textureFile );

                if ( !resolvedTextureFile.empty() )
                {
                    const std::string resolvedTextureKey = toLower( resolvedTextureFile );

                    TextureHandle textureHandle;
                    auto texIt = textureByFile.find( resolvedTextureKey );
                    if ( texIt == textureByFile.end() )
                    {
                        textureHandle = resources.createTexture(
                            "tex_" + makeSafeName( resolvedTextureFile ),
                            ( fs::path( "./assets/textures" ) / resolvedTextureFile ).generic_string() );
                        textureByFile.emplace( resolvedTextureKey, textureHandle );
                    }
                    else
                    {
                        textureHandle = texIt->second;
                    }

                    auto matIt = materialByFile.find( resolvedTextureKey );
                    if ( matIt == materialByFile.end() )
                    {
                        MaterialCreateInfo materialInfo;
                        materialInfo.baseColor = glm::vec4( 1.0f );
                        materialInfo.metallic = 0.0f;
                        materialInfo.roughness = 1.0f;
                        materialInfo.baseColorTexture = textureHandle;

                        loadedMesh.material = resources.createMaterial(
                            "mat_" + makeSafeName( resolvedTextureFile ),
                            materialInfo );
                        materialByFile.emplace( resolvedTextureKey, loadedMesh.material );
                    }
                    else
                    {
                        loadedMesh.material = matIt->second;
                    }

                    loadedMesh.hasMaterial = true;
                    ++meshesWithMaterial;
                }
            }
        }

        if ( !loadedMesh.hasMaterial )
            ++meshesWithoutMaterial;

        _loadedMeshes.push_back( loadedMesh );
    }

    std::cout << "Meshes cargadas: " << _loadedMeshes.size()
              << " | Con material: " << meshesWithMaterial
              << " | Sin material (textura faltante o no definida): "
              << meshesWithoutMaterial << "\n";

    const std::string grassBaseColorFile = resolveTextureFile( "Poliigon_GrassPatchyGround_4585_BaseColor.jpg" );
    const std::string grassNormalFile = resolveTextureFile( "Poliigon_GrassPatchyGround_4585_Normal.png" );

    if ( grassBaseColorFile.empty() )
    {
        throw std::runtime_error( "No se encontro la textura base del suelo de grass." );
    }

    TextureHandle grassBaseColorTexture = textureByFile.count( toLower( grassBaseColorFile ) )
        ? textureByFile[toLower( grassBaseColorFile )]
        : resources.createTexture(
            "tex_" + makeSafeName( grassBaseColorFile ),
            ( fs::path( "./assets/textures" ) / grassBaseColorFile ).generic_string() );

    if ( textureByFile.count( toLower( grassBaseColorFile ) ) == 0 )
        textureByFile.emplace( toLower( grassBaseColorFile ), grassBaseColorTexture );

    TextureHandle grassNormalTexture;
    const bool hasGrassNormal = !grassNormalFile.empty();
    if ( hasGrassNormal )
    {
        grassNormalTexture = textureByFile.count( toLower( grassNormalFile ) )
            ? textureByFile[toLower( grassNormalFile )]
            : resources.createTexture(
                "tex_" + makeSafeName( grassNormalFile ),
                ( fs::path( "./assets/textures" ) / grassNormalFile ).generic_string() );

        if ( textureByFile.count( toLower( grassNormalFile ) ) == 0 )
            textureByFile.emplace( toLower( grassNormalFile ), grassNormalTexture );
    }

    MaterialCreateInfo floorMatInfo;
    floorMatInfo.baseColor = glm::vec4( 1.0f );
    floorMatInfo.metallic = 0.0f;
    floorMatInfo.roughness = 0.95f;
    floorMatInfo.baseColorTexture = grassBaseColorTexture;
    if ( hasGrassNormal )
        floorMatInfo.normalTexture = grassNormalTexture;
    _floorMat = resources.createMaterial( "mat_floor_plane", floorMatInfo );
}

void App::populateScene()
{
    Scene& scene = _engine.getScene();

    _sceneEntities.clear();

    for ( const LoadedMeshData& loadedMesh : _loadedMeshes )
    {
        const MaterialHandle material = loadedMesh.hasMaterial
            ? loadedMesh.material
            : MaterialHandle{};

        _sceneEntities.push_back(
            scene.createEntity( loadedMesh.mesh, material, Transform{} ) );
    }

    _sceneEntities.push_back(
        scene.createEntity(
            _floorMesh,
            _floorMat,
            Transform{
                glm::vec3( 50.0f, -20.0f, 0.0f ),
                glm::vec3( 0.0f ),
                glm::vec3( 100.0f, 1.0f, 100.0f )
            } ) );

    // --- Camara -------------------------------------------------------------
    CameraHandle& camera = scene.getCamera();
    camera.setPosition( { 65.0f, 40.0f, -20.0f } );
    camera.setRotation( { glm::radians( 25.0f ), glm::radians( 70.0f ), 0.0f } );
    camera.setFOV( 60.0f );
    camera.setFarPlane( 1000.0f );

    // --- Iluminacion --------------------------------------------------------

    auto l =scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( -1.0f, -1.0f, -0.5f ) ),
        glm::vec3( 1.0f, 0.9f, 0.82f ),
        0.5f
    );

    scene.setMainLight(l);

    scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( -1.0f, 1.0f, 0.5f ) ),
        glm::vec3( 1.0f, 0.9f, 0.82f ),
        0.01f
    );

    const std::vector<float> wallLightXs = {
        -2.5f, 3.0f, 8.5f, 14.0f, 22.0f, 40.0f, 50.0f, 56.5f, 63.0f
    };

    auto createPointLightWithMarker = [&]( const glm::vec3& position,
                                           const glm::vec3& color,
                                           float intensity,
                                           float range )
    {
        scene.createLight( LightType::Point, position, color, intensity, range );
    };

    for ( const float x : wallLightXs )
    {
        createPointLightWithMarker(
            glm::vec3( x, 6.5f, 21.0f ),
            glm::vec3( 1.0f, 0.50f, 0.30f ),
            4.0f,
            20.5f );

        createPointLightWithMarker(
            glm::vec3( x, 6.5f, -26.0f ),
            glm::vec3( 1.0f, 0.50f, 0.30f ),
            4.0f,
            20.5f );
    }

    createPointLightWithMarker(
        glm::vec3( 18.4f, 20.0f, -2.5f ),
        glm::vec3( 1.0f, 0.80f, 0.60f ),
        20.0f,
        200.0f );

    createPointLightWithMarker(
        glm::vec3( 30.0f, 10.0f, -2.5f ),
        glm::vec3( 1.0f, 0.50f, 0.30f ),
        10.0f,
        40.0f );

    createPointLightWithMarker(
        glm::vec3( 50.0f, 10.0f, -2.5f ),
        glm::vec3( 1.0f, 0.80f, 0.60f ),
        10.0f,
        60.0f );

    // Lampara central elevada: 14 luces puntuales en circulo.
    const glm::vec3 chandelierCenter( 18.4f, 29.0f, -2.5f );
    const int chandelierLightCount = 14;
    const float chandelierRadius = 5.7f;
    const float twoPi = 6.28318530718f;

    for ( int i = 0; i < chandelierLightCount; ++i )
    {
        const float angle = twoPi * static_cast<float>( i ) / static_cast<float>( chandelierLightCount );
        const glm::vec3 lightPos(
            chandelierCenter.x + std::cos( angle ) * chandelierRadius,
            chandelierCenter.y,
            chandelierCenter.z + std::sin( angle ) * chandelierRadius );

        createPointLightWithMarker(
            lightPos,
            glm::vec3( 1.0f, 0.50f, 0.30f ),
            50.0f,
            5.0f );
    }

    createPointLightWithMarker(
        glm::vec3( -20.0f, 43.0f, -2.5f ),
        glm::vec3( 0.85f, 0.9f, 1.0f ),
        10.0f,
        70.0f );

    createPointLightWithMarker(
        glm::vec3( 65.0f, 50.0f, -2.5f ),
        glm::vec3( 0.85f, 0.9f, 1.0f ),
        10.0f,
        70.0f );


    const std::vector<float> windowLightZs = {
        -22.5f,
        -12.5f,
        -7.5f,
        -2.5f,
        3.5f,
        8.5f,
        18.5f
    };

    for ( const float z : windowLightZs )
    {
        createPointLightWithMarker(
            glm::vec3( -20.0f, 20.0f, z ),
            glm::vec3( 1.0f, 0.8f, 0.6f ),
            5.0f,
            40.0f );
    }

    createPointLightWithMarker(
        glm::vec3( 0.0f, 20.0f, -2.5f ),
        glm::vec3( 1.0f, 0.80f, 0.60f ),
        20.0f,
        25.0f );

    createPointLightWithMarker(
        glm::vec3( -10.0f, 20.0f, -2.5f ),
        glm::vec3( 1.0f, 0.80f, 0.60f ),
        10.0f,
        20.0f );

    createPointLightWithMarker(
        glm::vec3( 10.2f, 6.0f, -10.5f ),
        glm::vec3( 1.0f, 0.05f, 0.05f ),
        50.0f,
        2.0f );

    createPointLightWithMarker(
        glm::vec3( 10.2f, 6.0f, 5.6f ),
        glm::vec3( 1.0f, 0.05f, 0.05f ),
        50.0f,
        2.0f );
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

    (void)deltaTime;
}

void App::freeScene()
{
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
