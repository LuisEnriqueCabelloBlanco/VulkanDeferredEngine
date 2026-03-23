#include "App.h"
#include <fstream>


void App::start()
{
    _mainCamera = &_engine.getMainCamera();
    loadModels();
    _engine.createDirectionalLight( glm::vec3( 0, -1, 5 ), glm::vec3(0,0,1), 1 );
    _engine.createPointLight( glm::vec3( 0, 0, -1 ), glm::vec3( 1, 0, 0 ), 1 );
    _engine.createPointLight( glm::vec3( 1, 0, -1 ), glm::vec3( 0, 1, 0 ), 1 );
    _engine.createPointLight( glm::vec3( -1, 0, -1 ), glm::vec3( 0, 0, 1 ), 1 );
    
}

void App::mainLoop() {
    start();
    SDL_SetRelativeMouseMode( SDL_TRUE );

    bool running = true;
    while (running)
    {
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
                    SDL_SetRelativeMouseMode( SDL_FALSE );
                }
            }

            if (ev.type == SDL_MOUSEMOTION) {
                _mainCamera->rotateY( ev.motion.xrel * 0.1 );
            }
            if (ev.type == SDL_QUIT) {
                running = false;
            }
            //if (ev.type == SDL_WINDOWEVENT) {
            //    _window.handleWindowEvent( ev.window );
            //}
        }
        //glfwPollEvents();
        update();

        _engine.drawFrame( objects );

    }
}

void App::update()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

    objects[0].modelMatrix = glm::translate( glm::rotate( glm::mat4( 1 ), glm::radians( -10.f ), glm::vec3( 0, 1, 0 ) ), glm::vec3( sin( time ) * 1.5f, 0, 0 ) );
    objects[1].modelMatrix = glm::translate(glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 180.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) ),glm::vec3(0,-1,0) );
    objects[2].modelMatrix = glm::scale( glm::translate( glm::mat4( 1 ), glm::vec3( 0, -1, 0 ) ) , glm::vec3( 10 ) );

    glm::vec3 right = glm::cross( glm::vec3( 0, 1, 0 ), _mainCamera->getDirection()  );


    glm::vec3 newPos = _mainCamera->getPos() + right * _moveDir.x + _mainCamera->getDirection() * _moveDir.z;
    
    _mainCamera->setPosition( newPos );
}



void App::loadModels()
{

    int characterTextureHandle = _engine.loadTexture( TEXTURE_PATH );
    int whiteTextureHandle = _engine.loadTexture( TEXTURE2_PATH );
    int koreanoTextureHandle = _engine.loadTexture( TEXTURE3_PATH );


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

    vertices.push_back( v1 );
    vertices.push_back( v2 );
    vertices.push_back( v3 );

    MaterialData mat1;

    mat1.metallic = 0;
    mat1.roughtness = 0.95;
    mat1.texutreIndex = characterTextureHandle;


    MaterialData mat2;

    mat2.metallic = 0.1;
    mat2.roughtness = 0.25;
    mat2.texutreIndex = koreanoTextureHandle;

    MaterialData mat3;

    mat3.metallic = 0.7;
    mat3.roughtness = 0.3;
    mat3.texutreIndex = whiteTextureHandle;

    Mesh* mesh = _engine.createMesh( vertices );

    objects.push_back( { mesh, glm::mat4( 1 ),mat3 });

    Mesh* mesh2 = _engine.createMesh(MODEL_PATH );

    objects.push_back( { mesh2, glm::mat4( 1 ),mat1 } );

    Mesh* mesh3 = _engine.createMesh(MODEL_PATH2);

    objects.push_back( { mesh3, glm::mat4( 10 ),mat2 } );

    mesh2 = _engine.createMesh( MODEL_PATH3 );
    objects.push_back( { mesh2, glm::translate(glm::mat4(1),glm::vec3(2.5,0,0)),mat2 });
    mesh2 = _engine.createMesh( *mesh2 );
    objects.push_back( { mesh2, glm::translate(glm::mat4(1),glm::vec3(-2.5,0,0)),mat2 });

    for (int i = 0; i < 5;i++) {
        for (int j = 0; j < 5; j++) {
            mesh2 = _engine.createMesh( *mesh2 );
            mat3.roughtness = i * 0.1;
            mat3.metallic = j * 0.1;
            objects.push_back( { mesh2, glm::translate( glm::mat4( 1 ),glm::vec3( -5 + i * -2.5,0,j*2.5 ) ),mat3 } );
        }
    }
    
}

void App::freeObjects()
{
    for (auto obj : objects) {
        delete obj.mesh;
    }
}


App::App()
{
   /* _window.init("Proyecto Vulkan",WIDTH,HEIGHT,_instance);*/
}

