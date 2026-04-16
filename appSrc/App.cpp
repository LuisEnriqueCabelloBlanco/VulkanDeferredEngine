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
                _mainCamera->rotateY( ev.motion.xrel * 0.1 );
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

    int characterTextureHandle = _engine.loadTexture( TEXTURE_PATH );
    int whiteTextureHandle = _engine.loadTexture( TEXTURE2_PATH );
    int koreanoTextureHandle = _engine.loadTexture( TEXTURE3_PATH );
    int normalTexture = _engine.loadTexture( NORMAL_TEXTURE_PATH );
    int wallColor = _engine.loadTexture( WALL_TEXTURE_PATH );
    int wallNormal = _engine.loadTexture( WALL_NORMAL_TEXTURE_PATH );

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

    v1.tangent = glm::vec3(normalize(v1.pos-v2.pos));
    v2.tangent = glm::vec3(normalize(v1.pos-v2.pos));
    v3.tangent = glm::vec3(normalize(v1.pos-v2.pos));


    vertices.push_back( v1 );
    vertices.push_back( v2 );
    vertices.push_back( v3 );

    MaterialData mat1;

    mat1.metallic = 0;
    mat1.roughtness = 0.95;
    mat1.texutreIndex = characterTextureHandle;
    mat1.normalTextureIndex =normalTexture;


    MaterialData mat2;

    mat2.metallic = 0.1;
    mat2.roughtness = 0.25;
    mat2.texutreIndex = koreanoTextureHandle;
    mat2.normalTextureIndex = wallNormal;

    MaterialData mat3;

    mat3.metallic = 0.01;
    mat3.roughtness = 0.8;
    mat3.texutreIndex = wallColor;
    mat3.normalTextureIndex = wallNormal;

    MaterialData planeMat;

    planeMat.metallic = 0;
    planeMat.roughtness = 0.5;
    planeMat.texutreIndex = whiteTextureHandle;
    planeMat.normalTextureIndex = -1;


    //TODO crear gestor de recursos
    triangle = _engine.createMesh( vertices );
    character = _engine.createMesh(MODEL_PATH );
    esfera = _engine.createMesh( MODEL_PATH3 );
    planoSincolor = _engine.createMesh(MODEL_PATH4);

    
    objects.push_back( { triangle, glm::mat4( 1 ),mat3 });
    objects.push_back( { character, glm::mat4( 1 ),mat1 } );
    //objects.push_back( { planoSincolor, glm::mat4( 10 ),planeMat } );

    objects.push_back( { esfera, glm::translate(glm::mat4(1),glm::vec3(2.5,0,0)),mat2 });;
    objects.push_back( { esfera, glm::translate(glm::mat4(1),glm::vec3(-2.5,0,0)),mat2 });

    for (int i = 0; i < 100;i++) {
        for (int j = 0; j < 100; j++) {
            mat3.roughtness = std::min(i * 0.1,1.0);
            mat3.metallic =std::min( j * 0.1,1.0);
            objects.push_back( { esfera, glm::translate( glm::mat4( 1 ),glm::vec3( -5 + i * -5,0,j*5 ) ),mat3 } );
        }
    }

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            objects.push_back( { planoSincolor, glm::scale( glm::translate( glm::mat4( 1 ), glm::vec3( i * -5, -1, j * 5 ) ) , glm::vec3( 10 ) ),planeMat } );
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
    delete triangle;
    delete character;
    delete esfera;
    delete planoSincolor;
}


App::App()
{
   /* _window.init("Proyecto Vulkan",WIDTH,HEIGHT,_instance);*/
}

