#include "App.h"

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ---------------------------------------------------------------------------
// Rutas de assets. Ajusta segun el contenido de tu carpeta assets/.
// Las rutas son relativas al directorio del proyecto.
// ---------------------------------------------------------------------------
static const std::string MESH_CUBE  = "./assets/meshes/cubo.obj";
static const std::string MESH_PLANE = "./assets/meshes/plano.obj";
static const std::string MESH_SPHERE = "./assets/meshes/esfera.obj";

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

    // --- Mallas -------------------------------------------------------------
    _cubeMesh  = resources.createMesh( "mesh_cube",  MESH_CUBE  );
    _planeMesh = resources.createMesh( "mesh_plane", MESH_PLANE );

    // --- Materiales ---------------------------------------------------------
    MaterialCreateInfo cubeMatInfo;
    cubeMatInfo.baseColor = glm::vec4( 0.7f, 0.7f, 0.75f, 1.0f );
    cubeMatInfo.metallic  = 0.0f;
    cubeMatInfo.roughness = 0.6f;
    _cubeMat = resources.createMaterial( "mat_cube", cubeMatInfo );

    MaterialCreateInfo planeMatInfo;
    planeMatInfo.baseColor = glm::vec4( 0.1f, 0.1f, 0.1f, 1.0f );
    planeMatInfo.metallic  = 0.0f;
    planeMatInfo.roughness = 0.85f;
    _planeMat = resources.createMaterial( "mat_plane", planeMatInfo );

    // Aniade aqui mas mallas, texturas y materiales.
    // Los handles guardados como miembros estaran disponibles en populateScene().
}

void App::populateScene()
{
    Scene& scene = _engine.getScene();

    // --- Entidades ----------------------------------------------------------

    // Cubo centrado en el origen, a ras de suelo
    _cubeEntity = scene.createEntity(
        _cubeMesh,
        _cubeMat,
        Transform{
            glm::vec3( 0.0f, 1.0f, 0.0f ),               // elevado una unidad
            glm::vec3( 0.0f, glm::radians(45.0f), 0.0f), // rotado 45 grados
            glm::vec3( 1.0f )
        }
    );

    // Plano de suelo: grande y plano en Y = 0
    _planeEntity = scene.createEntity(
        _planeMesh,
        _planeMat,
        Transform{
            glm::vec3( 0.0f, 0.0f, 0.0f ),   // a ras del suelo
            glm::vec3( 0.0f ),
            glm::vec3( 10.0f, 1.0f, 10.0f )    // amplio en X y Z, neutro en Y
        }
    );

    // --- Camara -------------------------------------------------------------
    CameraHandle& camera = scene.getCamera();
    camera.setPosition( { 0.0f, 4.0f, -5.0f } );
    camera.setRotation( { glm::radians( 35.0f ), 0.0f, 0.0f } );
    camera.setFOV( 60.0f );

    // --- Iluminacion --------------------------------------------------------

    // Luz direccional principal: angulo alto desde la izquierda-frente
    _mainLight = scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( -1.0f, -1.5f, 1.0f ) ),  // direccion
        glm::vec3( 1.0f, 0.85f, 0.75f ),                     // blanco calido
        0.3f
    );

    // Se setea la luz direccional como luz principal, para que proyecte sombras.
    scene.setMainLight(_mainLight);

    // Point light derecha
    scene.createLight(
        LightType::Point,
        glm::vec3( -5.0f, 3.0f, 1.0f ),    // posicion elevada a la derecha
        glm::vec3( 0.6f, 0.7f, 1.0f ),       // tono azul frio para contrastar
        3.0f,
        10.0f                                 // rango
    );

    // Point light fondo
    scene.createLight(
        LightType::Point,
        glm::vec3(  1.0f, 1.5f,  5.0f ),    // posicion centrada al fondo
        glm::vec3( 1.0f, 0.85f, 0.6f ),      // tono naranja calido
        5.0f,
        10.0f
    );

    // Aniade aqui mas entidades y luces usando los handles de loadResources().
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

    // Aniade aqui la logica de juego de cada frame.
    // Ejemplo: rotar el cubo lentamente.
    // if ( _cubeEntity.isValid() )
    //     _cubeEntity.rotateY( glm::radians( 30.0f ) * deltaTime );
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
