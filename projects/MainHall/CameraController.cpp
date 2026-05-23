#include "CameraController.h"

#include <glm/glm.hpp>
#include <algorithm>

void CameraController::processEvent( const SDL_Event& ev )
{
    if ( ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP )
    {
        const bool pressed = ( ev.type == SDL_KEYDOWN );

        switch ( ev.key.keysym.scancode )
        {
            case SDL_SCANCODE_D: _keys[0] = pressed; break;
            case SDL_SCANCODE_A: _keys[1] = pressed; break;
            case SDL_SCANCODE_W: _keys[2] = pressed; break;
            case SDL_SCANCODE_S: _keys[3] = pressed; break;

            case SDL_SCANCODE_ESCAPE:
                if ( pressed )
                {
                    SDL_bool current = SDL_GetRelativeMouseMode();
                    SDL_SetRelativeMouseMode( current ? SDL_FALSE : SDL_TRUE );
                }
                break;

            default: break;
        }
    }

    if ( ev.type == SDL_MOUSEMOTION )
    {
        _pendingYaw   += static_cast<float>( ev.motion.xrel ) * _mouseSensitivity;
        _pendingPitch += static_cast<float>( ev.motion.yrel ) * _mouseSensitivity;
    }

    if ( ev.type == SDL_MOUSEWHEEL )
    {
        _movementSpeed += ev.wheel.preciseY;
        _movementSpeed  = std::max( 0.0f, _movementSpeed );
    }
}

void CameraController::update( CameraHandle& camera, float deltaTime )
{
    // Rotacion acumulada del raton
    if ( _pendingYaw != 0.0f || _pendingPitch != 0.0f )
    {
        camera.rotateY( glm::radians( _pendingYaw ) );
        camera.rotateX( glm::radians( _pendingPitch ) );
        _pendingYaw   = 0.0f;
        _pendingPitch = 0.0f;
    }

    // Movimiento por teclado en ejes locales de la camara
    glm::vec3 moveDir( 0.0f );
    if ( _keys[0] ) moveDir += glm::vec3( -1,  0, 0 ); // D -> derecha
    if ( _keys[1] ) moveDir += glm::vec3(  1,  0, 0 ); // A -> izquierda
    if ( _keys[2] ) moveDir += glm::vec3(  0,  0, 1 ); // W -> adelante
    if ( _keys[3] ) moveDir += glm::vec3(  0,  0,-1 ); // S -> atras

    if ( moveDir != glm::vec3( 0.0f ) )
    {
        moveDir = glm::normalize( moveDir );

        glm::vec3 right = glm::normalize(
            glm::cross( glm::vec3( 0, 1, 0 ), camera.getForward() )
        );

        glm::vec3 move = right * moveDir.x
                       + glm::normalize( camera.getForward() ) * moveDir.z;

        camera.setPosition( camera.getPosition() + move * deltaTime * _movementSpeed );
    }
}
