#pragma once

/**
 * @file WindowEvent.h
 * @brief Contrato de evento de ventana desacoplado.
 *
 * El motor tiene una capa de contrato de eventos desacoplada. Necesita que
 * la aplicacion traduzca los eventos nativos a WindowEvent y los entregue al
 * motor mediante EngineAPI::handleWindowEvent().
 */

/**
 * @brief Tipo de evento de ventana que puede recibir el motor.
 */
enum class WindowEventType {
    Unknown, ///< Evento no reconocido o no inicializado.
    Resized, ///< La ventana ha cambiado de tamano. Rellena width y height.
};

/**
 * @brief Descripcion de un evento de ventana.
 *
 * La aplicacion construye un WindowEvent a partir de un evento nativo y lo
 * entrega al motor con EngineAPI::handleWindowEvent(). El motor actualiza
 * su estado interno (por ejemplo, recrea el swapchain) en funcion del tipo.
 *
 * Ejemplo de uso:
 * @code
 * case SDL_WINDOWEVENT_RESIZED:
 *     WindowEvent ev;
 *     ev.type   = WindowEventType::Resized;
 *     ev.width  = sdlEvent.window.data1;
 *     ev.height = sdlEvent.window.data2;
 *     engine.handleWindowEvent(ev);
 *     break;
 * @endcode
 */
struct WindowEvent {
    WindowEventType type = WindowEventType::Unknown; ///< Tipo del evento.
    int width  = 0; ///< Nuevo ancho en pixels (relevante para Resized).
    int height = 0; ///< Nuevo alto en pixels (relevante para Resized).
};
