#pragma once

#include <SDL2/SDL_events.h>
#include "EngineAPI.h"

/**
 * Gestiona los eventos de ventana SDL relevantes para el motor.
 *
 * Traduce SDL_WINDOWEVENT_RESIZED al WindowEvent del motor y
 * detecta SDL_QUIT para indicar al bucle principal que debe terminar.
 *
 * Uso tipico:
 *   WindowHandler wh( engine );
 *   while ( running ) {
 *       SDL_Event ev;
 *       while ( SDL_PollEvent( &ev ) ) {
 *           running = wh.processEvent( ev );
 *           ...
 *       }
 *   }
 */
class WindowHandler {
public:
    /**
     * @param engine Referencia al motor. Debe sobrevivir al WindowHandler.
     */
    explicit WindowHandler( EngineAPI& engine ) : _engine( engine ) {}

    /**
     * Procesa un evento SDL.
     * @return false si se ha recibido SDL_QUIT y el bucle debe terminar,
     *         true en cualquier otro caso.
     */
    bool processEvent( const SDL_Event& ev );

private:
    EngineAPI& _engine;
};
