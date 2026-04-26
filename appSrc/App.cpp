#include "App.h"
#include <fstream>
#include <chrono>
#include<algorithm>
#include <glm/gtc/matrix_transform.hpp>

void App::safeCleanup( bool& initialized ) noexcept {
    if (!initialized) {
        return;
    }

    try {
        _engine.wait();
        freeObjects();
        _engine.cleanup();
    }
    catch (...) {
    }

    initialized = false;
}

bool App::run() {
    bool initialized = false;

    try {
        _engine.init( "AppExample" );
        initialized = true;

        mainLoop();

        safeCleanup( initialized );
        return true;
    }
    catch (const SceneException& e) {
        std::cerr << "Scene error [" << static_cast<int>( e.code() ) << "]: " << e.what() << "\n";
        safeCleanup( initialized );
    }
    catch (const std::exception& e) {
        std::cerr << "Engine runtime error: " << e.what() << "\n";
        safeCleanup( initialized );
    }
    catch (...) {
        std::cerr << "Unknown fatal error\n";
        safeCleanup( initialized );
    }

    return false;
}

void App::start()
{

    _mainCamera = &_engine.getMainCamera();
    auto startTime = std::chrono::high_resolution_clock::now();
    loadModels();
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Tiempo en cargar los modelos: "<<
        std::chrono::duration<float, std::chrono::seconds::period>( endTime - startTime ).count() << "\n";


    startTime = std::chrono::high_resolution_clock::now();
    addLighting();
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Tiempo en cargar las luces: " <<
        std::chrono::duration<float, std::chrono::seconds::period>( endTime - startTime ).count() << "\n";
}

void App::mainLoop() {
    static auto startTime = std::chrono::high_resolution_clock::now();
    start();
    SDL_SetRelativeMouseMode( SDL_TRUE );

    bool running = true;
    while (running)
    {
        auto frameStart = std::chrono::high_resolution_clock::now();

        SDL_Event ev;
        _moveDir = glm::vec3( 0 );
        while (SDL_PollEvent( &ev )) {

            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.scancode == SDL_SCANCODE_D) {
                    _moveDir += glm::vec3( -1, 0, 0 );
                }
                if (ev.key.keysym.scancode == SDL_SCANCODE_A) {
                    _moveDir += glm::vec3( 1, 0, 0 );
                }
                if (ev.key.keysym.scancode == SDL_SCANCODE_W) {
                    _moveDir += glm::vec3( 0, 0, 1 );
                }
                if (ev.key.keysym.scancode == SDL_SCANCODE_S) {
                    _moveDir += glm::vec3( 0, 0, -1 );
                }
                if (ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    SDL_bool b = SDL_GetRelativeMouseMode();
                    SDL_SetRelativeMouseMode((SDL_bool)(((int)b+1)%2) );
                }
            }

            if (ev.type == SDL_MOUSEMOTION) {
                _mainCamera->rotateY( static_cast<float>( ev.motion.xrel ) * 0.1f );
            }
            if (ev.type == SDL_QUIT) {
                running = false;
            }
            if (ev.type == SDL_WINDOWEVENT) {
                WindowEvent out;
                out.type = WindowEventType::Unknown;
                out.width = ev.window.data1;
                out.height = ev.window.data2;

                if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                    out.type = WindowEventType::Resized;
                }

                _engine.handleWindowEvent( out );
            }
        }

        update();

        _engine.drawFrame();

        auto endFrame = std::chrono::high_resolution_clock::now();

        _counter++;

        _deltaTime = std::chrono::duration<float, std::chrono::seconds::period>( endFrame - frameStart ).count();

    }
}

void App::update()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

    if (_triangleEntity.isValid()) {
        _triangleEntity.setTransform(
            glm::vec3( sin( time ) * 1.5f, 0.0f, 0.0f ),
            glm::vec3( 0.0f, glm::radians( -10.0f ), 0.0f )
        );
    }

    if (_characterEntity.isValid()) {
        _characterEntity.setTransform(
            glm::vec3( 0.0f, -1.0f, 0.0f ),
            glm::vec3( 0.0f, time * glm::radians( 180.0f ), 0.0f )
        );
    }


    glm::vec3 right = glm::cross( glm::vec3( 0, 1, 0 ), _mainCamera->getDirection()  );


    glm::vec3 newPos = _mainCamera->getPos() + (right * _moveDir.x + _mainCamera->getDirection() * _moveDir.z)*_deltaTime *10.f;

    _mainCamera->setPosition( newPos );
}



void App::loadModels()
{
    Scene& scene = _engine.getScene();
    ResourceManager& resources = _engine.getResourceManager();

    TextureHandle characterTextureHandle = resources.createTexture( "tex_character_basecolor", TEXTURE_PATH );
    TextureHandle whiteTextureHandle = resources.createTexture( "tex_white", TEXTURE2_PATH );
    TextureHandle koreanoTextureHandle = resources.createTexture( "tex_koreano", TEXTURE3_PATH );
    TextureHandle normalTexture = resources.createTexture( "tex_character_normal", NORMAL_TEXTURE_PATH );
    TextureHandle wallColor = resources.createTexture( "tex_wall_basecolor", WALL_TEXTURE_PATH );
    TextureHandle wallNormal = resources.createTexture( "tex_wall_normal", WALL_NORMAL_TEXTURE_PATH );

    std::vector<Vertex> vertices;
    std::vector<Vertex> vertices2;

    Vertex v1;
    Vertex v2;
    Vertex v3;

    int dim = 1;

    v1.pos = glm::vec3( -dim, 0, 0);
    v2.pos = glm::vec3( 0, dim, 0 );
    v3.pos = glm::vec3( dim, 0, 0);

    v1.color = glm::vec3( 1.f, 1.0f, 1.f);
    v2.color = glm::vec3( 1.f, 1.f, 1.f);
    v3.color = glm::vec3( 1.f, 1.f, 1.f);

    v1.normal = glm::vec3( 0.f, 0.0f, -1.f );
    v2.normal = glm::vec3( 0.f, 0.0f, -1.f );
    v3.normal = glm::vec3( 0.f, 0.0f, -1.f );

    v1.texCoord = glm::vec2(0,0);
    v2.texCoord = glm::vec2(2,0);
    v3.texCoord = glm::vec2(0,2);

    v1.tangent = glm::vec3( normalize( v1.pos - v2.pos ) );
    v2.tangent = glm::vec3( normalize( v1.pos - v2.pos ) );
    v3.tangent = glm::vec3( normalize( v1.pos - v2.pos ) );


    vertices.push_back( v1 );
    vertices.push_back( v2 );
    vertices.push_back( v3 );

    MaterialCreateInfo mat1;
    mat1.metallic = 0.0f;
    mat1.roughness = 0.95f;
    mat1.baseColorTexture = characterTextureHandle;
    mat1.normalTexture = normalTexture;

    MaterialCreateInfo mat2;
    mat2.metallic = 0.1f;
    mat2.roughness = 0.25f;
    mat2.baseColorTexture = koreanoTextureHandle;
    mat2.normalTexture = wallNormal;

    MaterialCreateInfo mat3;
    mat3.metallic = 0.01f;
    mat3.roughness = 0.8f;
    mat3.baseColorTexture = wallColor;
    mat3.normalTexture = wallNormal;

    MaterialCreateInfo planeMat;
    planeMat.metallic = 0.0f;
    planeMat.roughness = 0.5f;
    planeMat.baseColorTexture = whiteTextureHandle;

    MaterialHandle mat1Handle = resources.createMaterial( "mat_character", mat1 );
    MaterialHandle mat2Handle = resources.createMaterial( "mat_koreano", mat2 );
    MaterialHandle mat3Handle = resources.createMaterial( "mat_wall", mat3 );
    MaterialHandle planeMatHandle = resources.createMaterial( "mat_plane", planeMat );


    MeshHandle triangle = resources.createMesh( "mesh_triangle", vertices );
    MeshHandle character = resources.createMesh( "mesh_character", MODEL_PATH );
    MeshHandle esfera = resources.createMesh( "mesh_sphere", MODEL_PATH3 );
    MeshHandle planoSincolor = resources.createMesh( "mesh_plane_no_color", MODEL_PATH4 );

    _triangleEntity = scene.createEntity(
        triangle,
        mat3Handle,
        Transform{
            glm::vec3( 0.0f ),
            glm::vec3( 0.0f ),
            glm::vec3( 1.0f )
        }
    );

    _characterEntity = scene.createEntity(
        character,
        mat1Handle,
        Transform{
            glm::vec3( 0.0f ),
            glm::vec3( 0.0f ),
            glm::vec3( 1.0f )
        }
    );

    _sphereLeftEntity = scene.createEntity(
        esfera,
        mat2Handle,
        Transform{
            glm::vec3( 2.5f, 0.0f, 0.0f ),
            glm::vec3( 0.0f ),
            glm::vec3( 1.0f )
        }
    );

    _sphereRightEntity = scene.createEntity(
        esfera,
        mat2Handle,
        Transform{
            glm::vec3( -2.5f, 0.0f, 0.0f ),
            glm::vec3( 0.0f ),
            glm::vec3( 1.0f )
        }
    );

    for (int i = 0; i < 100;i++) {
        for (int j = 0; j < 100; j++) {
            MaterialCreateInfo dynamicMat = mat3;
            dynamicMat.roughness = std::min( static_cast<float>( i ) * 0.1f, 1.0f );
            dynamicMat.metallic = std::min( static_cast<float>( j ) * 0.1f, 1.0f );
            MaterialHandle dynamicMatHandle = resources.createMaterial( "mat_grid_" + std::to_string( i ) + "_" + std::to_string( j ), dynamicMat );

            scene.createEntity(
                esfera,
                dynamicMatHandle,
                Transform{
                    glm::vec3( -5.0f + static_cast<float>( i ) * -5.0f, 0.0f, static_cast<float>( j ) * 5.0f ),
                    glm::vec3( 0.0f ),
                    glm::vec3( 1.0f )
                }
            );
        }
    }

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            scene.createEntity(
                planoSincolor,
                planeMatHandle,
                Transform{
                    glm::vec3( static_cast<float>( i ) * -5.0f, -1.0f, static_cast<float>( j ) * 5.0f ),
                    glm::vec3( 0.0f ),
                    glm::vec3( 10.0f )
                }
            );
        }
    }

}

void App::addLighting()
{
    _engine.createDirectionalLight( glm::vec3( -2, -0.9, 4 ), glm::vec3( 0.1, 0.1, 0.1 ), 1, true );
    _engine.createPointLight( glm::vec3( 0, 0, -1 ), glm::vec3( 1, 0, 0 ), 1,10, true );
    _engine.createPointLight( glm::vec3( 1, 0, -1 ), glm::vec3( 0, 1, 0 ), 1,10, true );
    _engine.createPointLight( glm::vec3( -1, 0, -1 ), glm::vec3( 0, 0, 1 ), 1,10, true );


    for (int i = 0; i < 10;i++) {
        for (int j = 0; j < 10; j++) {
            _engine.createPointLight( glm::vec3( -5 + i * -5, -0.5, j * 5 - 1.5 ), glm::vec3( 0.01 * i, 0.01 * j, 1 ), 1, 10, true);
        }
    }
    _engine.setMainLight( 0 );

    _engine.updateLightBuffer();
}

void App::freeObjects()
{
    ResourceManager& resources = _engine.getResourceManager();
    _engine.getScene().clear();
    resources.releaseAllMaterials();
    resources.releaseAllTextures();
    resources.releaseAllMeshes();
}


App::App()
{
   /* _window.init("Proyecto Vulkan",WIDTH,HEIGHT,_instance);*/
}

