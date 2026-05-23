#include "WindowHandler.h"
#include "WindowEvent.h"

bool WindowHandler::processEvent( const SDL_Event& ev )
{
    if ( ev.type == SDL_QUIT )
    {
        return false;
    }

    if ( ev.type == SDL_WINDOWEVENT &&
         ev.window.event == SDL_WINDOWEVENT_RESIZED )
    {
        WindowEvent out;
        out.type   = WindowEventType::Resized;
        out.width  = ev.window.data1;
        out.height = ev.window.data2;
        _engine.handleWindowEvent( out );
    }

    return true;
}
