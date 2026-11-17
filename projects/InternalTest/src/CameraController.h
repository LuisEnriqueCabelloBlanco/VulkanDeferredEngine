#pragma once

#include <glm/glm.hpp>
#include <SDL2/SDL_events.h>

#include "CameraHandle.h"

/**
 * Controlador de camara sencillo basado en eventos SDL.
 *
 * Procesa eventos de teclado y raton y aplica el movimiento
 * directamente sobre el CameraHandle que se le pasa en cada update.
 *
 * Teclas:
 *   W / S  -> adelante / atras
 *   A / D  -> izquierda / derecha
 *   Rueda  -> ajusta la velocidad de movimiento
 *   Escape -> activa/desactiva el modo captura del raton
 *
 * Uso tipico:
 *   CameraController controller;
 *   controller.processEvent( sdlEvent );   // dentro del poll loop
 *   controller.update( camera, deltaTime ); // una vez por frame
 */
class CameraController {
public:
    CameraController() = default;

    /**
     * Procesa un evento SDL. Llamar dentro del bucle de SDL_PollEvent.
     * Solo consume eventos de teclado, raton y rueda; ignora el resto.
     */
    void processEvent( const SDL_Event& ev );

    /**
     * Aplica el movimiento acumulado a la camara.
     * @param camera    Camara a mover.
     * @param deltaTime Tiempo del frame en segundos.
     */
    void update( CameraHandle& camera, float deltaTime );

    /** Velocidad de movimiento actual (unidades/segundo). */
    float getMovementSpeed() const { return _movementSpeed; }

    /** Establece la velocidad de movimiento. */
    void  setMovementSpeed( float speed ) { _movementSpeed = speed; }

private:
    bool  _keys[4]          = { false, false, false, false }; // D A W S
    float _movementSpeed    = 5.0f;
    float _mouseSensitivity = 0.1f;
    float _pendingYaw       = 0.0f;
    float _pendingPitch     = 0.0f;
};
