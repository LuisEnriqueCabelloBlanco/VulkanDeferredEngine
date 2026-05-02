#pragma once

#include <cstdint>
#include <string>

#include "BufferObjectsData.h"
#include "Scene.h"

// ---------------------------------------------------------------------------
// LightEntityHandle
// ---------------------------------------------------------------------------

/*
LightEntityHandle es el token que la aplicacion recibe al crear una luz.

Encapsula el par (index, generation) que identifica un LightSlot en la Scene,
mas un puntero a la Scene que lo emitio. Toda mutacion y consulta se despacha
directamente al slot: el slot es la fuente de verdad, el handle es solo
un punto de acceso validado.

A diferencia de RenderEntityHandle, el handle NO almacena ninguna copia local
del estado de la luz. No hay transformacion intermedia que compilar; los campos
de LightObject son exactamente los datos que la aplicacion escribe y el
renderer lee.

El handle es move-only.

Uso tipico:
    LightEntityHandle sun = scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( -1, -1, 0 ) ),
        glm::vec3( 1, 1, 0.9f ),
        2.0f );
    sun.setIntensity( 1.5f );
    sun.setActive( false );
    // Al salir de scope NO destruye la luz; el caller decide cuando llamar destroyLight().
*/
class LightEntityHandle {
public:
    // --- Construccion y ciclo de vida del handle ----------------------------

    LightEntityHandle() = default;

    LightEntityHandle( const LightEntityHandle& ) = delete;
    LightEntityHandle& operator=( const LightEntityHandle& ) = delete;

    LightEntityHandle( LightEntityHandle&& other ) noexcept;
    LightEntityHandle& operator=( LightEntityHandle&& other ) noexcept;

    ~LightEntityHandle() = default;

    // Devuelve true si el handle fue emitido y la luz no ha sido destruida.
    bool isValid() const;

    // --- Mutacion (despacha directamente al slot) ---------------------------

    // Lanza SceneException(StaleHandle) si la luz ya fue destruida.

    void setPosOrDir ( const glm::vec3& posOrDir  );
    void setColor    ( const glm::vec3& color      );
    void setIntensity( float intensity             );
    void setRange    ( float range                 );
    void setType     ( LightType type              );
    void setActive   ( bool active                 );

    // --- Consulta (despacha directamente al slot) ---------------------------

    // Lanza SceneException(StaleHandle) si la luz ya fue destruida.

    glm::vec3 getPosOrDir()  const;
    glm::vec3 getColor()     const;
    float     getIntensity() const;
    float     getRange()     const;
    LightType getType()      const;
    bool      isActive()     const;

private:
    friend class Scene;

    LightEntityHandle( uint32_t index, uint32_t generation, Scene* scene )
        : _index( index ), _generation( generation ), _scene( scene ) {
    }

    void invalidate();

private:
    uint32_t  _index      = 0;
    uint32_t  _generation = 0;   // 0 == handle nunca inicializado
    Scene*    _scene      = nullptr;
};
