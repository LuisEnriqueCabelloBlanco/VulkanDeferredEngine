#include "App.h"
#include <fstream>
#include <chrono>
#include<algorithm>


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

        _engine.drawFrame( objects );

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

    objects[0].modelMatrix = glm::translate( glm::rotate( glm::mat4( 1 ), glm::radians( -10.f ), glm::vec3( 0, 1, 0 ) ), glm::vec3( sin( time ) * 1.5f, 0, 0 ) );
    objects[1].modelMatrix = glm::translate(glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 180.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) ),glm::vec3(0,-1,0) );


    glm::vec3 right = glm::cross( glm::vec3( 0, 1, 0 ), _mainCamera->getDirection()  );


    glm::vec3 newPos = _mainCamera->getPos() + (right * _moveDir.x + _mainCamera->getDirection() * _moveDir.z)*_deltaTime *10.f;

    _mainCamera->setPosition( newPos );
}



void App::loadModels()
{

    TextureHandle characterTextureHandle = _engine.createTexture( "tex_character_basecolor", TEXTURE_PATH );
    TextureHandle whiteTextureHandle = _engine.createTexture( "tex_white", TEXTURE2_PATH );
    TextureHandle koreanoTextureHandle = _engine.createTexture( "tex_koreano", TEXTURE3_PATH );
    TextureHandle normalTexture = _engine.createTexture( "tex_character_normal", NORMAL_TEXTURE_PATH );
    TextureHandle wallColor = _engine.createTexture( "tex_wall_basecolor", WALL_TEXTURE_PATH );
    TextureHandle wallNormal = _engine.createTexture( "tex_wall_normal", WALL_NORMAL_TEXTURE_PATH );

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

    MaterialDesc mat1;
    mat1.metallic = 0.0f;
    mat1.roughtness = 0.95f;
    mat1.baseColorTexture = characterTextureHandle;
    mat1.normalTexture = normalTexture;

    MaterialDesc mat2;
    mat2.metallic = 0.1f;
    mat2.roughtness = 0.25f;
    mat2.baseColorTexture = koreanoTextureHandle;
    mat2.normalTexture = wallNormal;

    MaterialDesc mat3;
    mat3.metallic = 0.01f;
    mat3.roughtness = 0.8f;
    mat3.baseColorTexture = wallColor;
    mat3.normalTexture = wallNormal;

    MaterialDesc planeMat;
    planeMat.metallic = 0.0f;
    planeMat.roughtness = 0.5f;
    planeMat.baseColorTexture = whiteTextureHandle;

    MaterialHandle mat1Handle = _engine.createMaterial( "mat_character", mat1 );
    MaterialHandle mat2Handle = _engine.createMaterial( "mat_koreano", mat2 );
    MaterialHandle mat3Handle = _engine.createMaterial( "mat_wall", mat3 );
    MaterialHandle planeMatHandle = _engine.createMaterial( "mat_plane", planeMat );


    //TODO crear gestor de recursos
    triangle = _engine.createMesh( "mesh_triangle", vertices );
    character = _engine.createMesh( "mesh_character", MODEL_PATH );
    esfera = _engine.createMesh( "mesh_sphere", MODEL_PATH3 );
    planoSincolor = _engine.createMesh( "mesh_plane_no_color", MODEL_PATH4 );


    objects.push_back( { triangle, glm::mat4( 1 ), mat3Handle });
    objects.push_back( { character, glm::mat4( 1 ), mat1Handle } );
    //objects.push_back( { planoSincolor, glm::mat4( 10 ),planeMat } );

    objects.push_back( { esfera, glm::translate( glm::mat4( 1.0f ), glm::vec3( 2.5f, 0.0f, 0.0f ) ), mat2Handle } );
    objects.push_back( { esfera, glm::translate( glm::mat4( 1.0f ), glm::vec3( -2.5f, 0.0f, 0.0f ) ), mat2Handle } );

    for (int i = 0; i < 100;i++) {
        for (int j = 0; j < 100; j++) {
            MaterialDesc dynamicMat = mat3;
            dynamicMat.roughtness = std::min( static_cast<float>( i ) * 0.1f, 1.0f );
            dynamicMat.metallic = std::min( static_cast<float>( j ) * 0.1f, 1.0f );
            MaterialHandle dynamicMatHandle = _engine.createMaterial( "mat_grid_" + std::to_string( i ) + "_" + std::to_string( j ), dynamicMat );
            objects.push_back( { esfera, glm::translate( glm::mat4( 1.0f ), glm::vec3( -5.0f + static_cast<float>( i ) * -5.0f, 0.0f, static_cast<float>( j ) * 5.0f ) ), dynamicMatHandle } );
        }
    }

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            objects.push_back( { planoSincolor, glm::scale( glm::translate( glm::mat4( 1.0f ), glm::vec3( static_cast<float>( i ) * -5.0f, -1.0f, static_cast<float>( j ) * 5.0f ) ), glm::vec3( 10.0f ) ), planeMatHandle } );
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
}


App::App()
{
   /* _window.init("Proyecto Vulkan",WIDTH,HEIGHT,_instance);*/
}

