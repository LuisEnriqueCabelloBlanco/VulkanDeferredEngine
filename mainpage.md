@mainpage Documentacion del motor

Bienvenido a la documentacion de **VulkanDeferredEngine**.

[Descargar documentacion en PDF](documentacion.pdf)

---

## Indice

- [Plataforma](#plataforma)
- [Arquitectura general](#arquitectura-general)
- [Aplicacion minima](#aplicacion-minima)
- [EngineAPI](#engineapi)
- [Gestion de recursos](#gestion-de-recursos)
- [Escena: entidades y luces](#escena-entidades-y-luces)
- [Camara](#camara)
- [Bucle principal y eventos de ventana](#bucle-principal-y-eventos-de-ventana)
- [Manejo de errores](#manejo-de-errores)
- [Limites de capacidad](#limites-de-capacidad)

---

## Plataforma

VulkanDeferredEngine esta dirigido a **Windows x64**. Requiere una GPU con soporte de Vulkan 1.3 o superior y los redistribuibles de Visual C++ correspondientes. No se soportan otras plataformas en esta version.

---

## Arquitectura general

El motor expone tres subsistemas a traves de una unica fachada, `EngineAPI`:

| Subsistema        | Acceso                              | Responsabilidad                                     |
|-------------------|-------------------------------------|-----------------------------------------------------|
| `ResourceManager` | `EngineAPI::getResourceManager()`   | Carga y libera meshes, texturas y materiales en GPU |
| `Scene`           | `EngineAPI::getScene()`             | Crea, destruye e itera entidades y luces            |
| `CameraHandle`    | `Scene::getCamera()`                | Controla la camara principal de la escena           |

La aplicacion nunca instancia estos subsistemas directamente. Solo necesita incluir `EngineAPI.h`.

---

## Aplicacion minima

El siguiente bloque muestra la estructura completa de una aplicacion minima: inicializacion, carga de recursos, escena, bucle principal y limpieza.

```cpp
#include "EngineAPI.h"

int main()
{
    // Inicializacion
    EngineAPI engine;
    engine.init( "MiApp" );

    // Acceso a los dos subsistemas principales
    ResourceManager& res   = engine.getResourceManager();
    Scene&           scene = engine.getScene();

    // Carga de recursos
    MeshHandle     mesh = res.createMesh    ( "cubo",     "cube.obj"    );
    TextureHandle  tex  = res.createTexture ( "albedo",   "albedo.png"  );

    // Creación material PBR
    MaterialCreateInfo matInfo;
    matInfo.baseColor        = { 1.f, 1.f, 1.f, 1.f }; // modulacion sobre la textura
    matInfo.metallic         = 0.0f;                    // no metalico
    matInfo.roughness        = 0.6f;                    // semirugoso
    matInfo.baseColorTexture = tex;                     // textura asignada
    MaterialHandle mat = res.createMaterial( "mat", matInfo );

    // Poblado de la escena
    RenderEntityHandle entity = scene.createEntity( mesh, mat, Transform{} );

    // Luz direccional (sol)
    LightEntityHandle sol = scene.createLight(
        LightType::Directional,
        glm::normalize( glm::vec3( -1, -2, -1 ) ),
        glm::vec3( 1.f ),
        2.0f
    );

    // Designar esta luz como shadow caster
    scene.setMainLight( sol );

    // Configuracion de la camara
    scene.getCamera().setPosition( { 0.f, 2.f, -5.f } );
    scene.getCamera().setFOV( 60.f );

    // Bucle principal
    bool running = true;
    float deltaTime = 0.0f;
    while ( running )
    {
        // La aplicación gestiona los eventos
        SDL_Event ev;
        while ( SDL_PollEvent( &ev ) )
        {
            if ( ev.type == SDL_QUIT ) running = false;

            // Se notifica al motor el evento de resize de ventana
            if ( ev.type == SDL_WINDOWEVENT &&
                 ev.window.event == SDL_WINDOWEVENT_RESIZED )
            {
                WindowEvent wev;
                wev.type   = WindowEventType::Resized;
                wev.width  = ev.window.data1;
                wev.height = ev.window.data2;
                engine.handleWindowEvent( wev );
            }
        }

        // Logica de la aplicacion (update)
        entity.rotateY( glm::radians( 30.f ) * deltaTime );

        // Renderizado
        engine.drawFrame();

        // Delta time
        auto frameEnd = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>( frameEnd - frameStart ).count();
    }

    // Limpieza
    engine.wait();  // Garantiza que no hay comandos pendientes en la GPU
    scene.clear();  // destruye todas las entidades y luces
    res.clear();    // libera todas las mallas, texturas y materiales
    engine.cleanup();

    return 0;
}
```

---

## EngineAPI

`EngineAPI` es la unica clase que la aplicacion necesita instanciar, y `EngineAPI.h` es el unico header que necesita incluir. El resto de cabeceras del motor son accesibles de forma transitiva a traves de el.

La clase encapsula el ciclo de vida completo del motor: inicializacion, bucle de renderizado y limpieza. Tambien es el punto de acceso a los dos subsistemas principales:

```cpp
EngineAPI engine;
engine.init( "Mi aplicacion" );

ResourceManager& res   = engine.getResourceManager();
Scene&           scene = engine.getScene();

// ... bucle principal ...

engine.wait();    // espera a que la GPU termine todo el trabajo en vuelo
engine.cleanup(); // libera todos los recursos internos del motor
```

#### engine.wait()

Debe llamarse antes de liberar recursos manualmente al salir. Garantiza que no hay comandos pendientes en la GPU en el momento de la destruccion.

#### engine.drawFrame()

Es la llamada central del bucle principal. Recoge el estado actual de la escena (entidades, luces y camara), ejecuta los pases de renderizado deferred y presenta el resultado en la ventana. Debe llamarse exactamente una vez por iteracion del bucle.

@note Cualquier llamada a los metodos del motor antes de `init()` o despues de `cleanup()` lanza `std::logic_error`.

---

## Gestion de recursos

El `ResourceManager` gestiona tres tipos de recursos de GPU. Todos se identifican mediante **handles opacos**: valores ligeros y copiables que el motor invalida automaticamente si el recurso es liberado, evitando punteros colgantes.

### Meshes

```cpp
// Desde fichero (OBJ y otros formatos soportados)
MeshHandle m1 = res.createMesh( "terreno", "terrain.obj" );

// Desde vertices en memoria (sin indices)
MeshHandle m2 = res.createMesh( "triangulo", vertices );

// Indexada
MeshHandle m3 = res.createMesh( "cubo", indices, vertices );
```

### Texturas

```cpp
TextureHandle albedo  = res.createTexture( "albedo",   "diffuse.png" );
TextureHandle normals = res.createTexture( "normales", "normal.png"  );
```

### Materiales PBR

Los materiales describen la apariencia fisica del objeto mediante un modelo PBR metallic-roughness. Se configuran mediante `MaterialCreateInfo`:

```cpp
MaterialCreateInfo info;
info.baseColor        = { 1.f, 0.8f, 0.2f, 1.f }; // color base RGBA
info.metallic         = 0.9f;                       // 0 = dielectrico, 1 = metalico
info.roughness        = 0.2f;                       // 0 = espejo, 1 = completamente rugoso
info.baseColorTexture = albedo;                     // textura de albedo (opcional)
info.normalTexture    = normals;                    // mapa de normales (opcional)

MaterialHandle mat = res.createMaterial( "oro", info );
```

### Lookup y liberacion

```cpp
// Buscar un recurso ya cargado por nombre (no lanza excepciones)
MeshHandle m = res.tryGetMeshHandle( "terreno" );

// Liberar en runtime (orden: primero materiales, despues texturas)
res.releaseMaterial( mat    );
res.releaseTexture ( albedo );

// Liberar todos los recursos de un tipo a la vez
res.clear();

// Enumerar recursos vivos (util para debug)
for ( const auto& name : res.getMeshNames() ) { /* ... */ }
```

@warning Una textura no puede liberarse mientras algun material vivo la referencia. El motor lanza `ResourceException(DependencyInUse)` para proteger la coherencia.

---

## Escena: entidades y luces

### Entidades renderizables

```cpp
// Crear con mesh, material y transform
RenderEntityHandle entity = scene.createEntity( mesh, mat,
    Transform( { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f } ) );

// Crear vacia y configurar despues
RenderEntityHandle e2 = scene.createEntity();
e2.setMesh    ( mesh );
e2.setMaterial( mat  );
```

**Transform:** la convencion de rotacion es Euler XYZ (pitch, yaw, roll) en radianes. Se soportan operaciones absolutas y relativas; multiples mutaciones en el mismo frame tienen coste unitario.

```cpp
// Setters absolutos
entity.setPosition ( { 3.f, 0.f, 0.f }              );
entity.setRotation ( { 0.f, glm::radians(45.f), 0.f } );
entity.setScale    ( 2.f                              );

// Mutaciones relativas
entity.translate( { 0.f, 0.1f, 0.f }   );
entity.rotateY  ( glm::radians( 1.f )   );
entity.scale    ( 0.99f                 );
```

**Visibilidad y activacion:**

```cpp
entity.setVisible( false ); // oculta sin eliminar de la escena
entity.setActive ( false ); // excluye del render queue y del culling
```

**Destruccion:**

```cpp
scene.destroyEntity( entity );
```

### Luces

El motor soporta tres tipos de luz definidos por `LightType`:

| Tipo          | `posOrDir`                            | Rango | Descripcion                    |
|---------------|---------------------------------------|-------|--------------------------------|
| `Directional` | Direccion normalizada hacia la fuente | No    | Luz global sin atenuacion      |
| `Point`       | Posicion en el mundo                  | Si    | Omnidireccional con atenuacion |
| `Spotlight`   | Posicion en el mundo                  | Si    | Cono de luz con atenuacion     |

```cpp
// Luz direccional (sol)
LightEntityHandle sol = scene.createLight(
    LightType::Directional,
    glm::normalize( glm::vec3( -1, -1, 0 ) ),
    glm::vec3( 1.f, 0.95f, 0.8f ),
    2.0f
);

// Luz puntual
LightEntityHandle bombilla = scene.createLight(
    LightType::Point,
    glm::vec3( 0.f, 3.f, 0.f ),    // posicion
    glm::vec3( 1.f, 0.8f, 0.4f ),  // color
    1.5f,                           // intensidad
    10.f                            // rango en unidades de mundo
);
```

Todos los parametros de una luz pueden modificarse en tiempo real:

```cpp
sol.setIntensity( 1.5f                      );
sol.setColor    ( { 1.f, 0.6f, 0.3f }       ); // amanecer
sol.setActive   ( false                     ); // apagar sin destruir
```

### Shadow caster (main light)

Una luz de tipo `Directional` puede designarse como **luz principal**, que es la unica que genera el shadow map de la escena:

```cpp
scene.setMainLight  ( sol ); // designa la luz principal (shadow caster)
scene.clearMainLight(     ); // elimina la designacion
scene.hasMainLight  (     ); // consulta si hay una designada
```

### Enumeracion de la escena

```cpp
scene.forEachEntity( [&]( RenderEntityHandle& e ) {
    e.translate( { 0.f, 0.001f, 0.f } );
} );

scene.forEachLight( [&]( LightEntityHandle& l ) {
    l.setIntensity( l.getIntensity() * 0.99f );
} );

std::cout << scene.entityCount() << " entidades, "
          << scene.lightCount()  << " luces\n";
```

@warning No crear ni destruir entidades o luces dentro del callback de `forEachEntity` / `forEachLight`.

---

## Camara

Hay exactamente una camara por escena, accesible mediante `scene.getCamera()`.

```cpp
CameraHandle& cam = scene.getCamera();

// Posicion y orientacion
cam.setPosition( { 0.f, 5.f, -10.f }      );
cam.setYaw      ( glm::radians( 180.f )    );
cam.setPitch    ( glm::radians( -15.f )    );

// Proyeccion
cam.setFOV     ( 75.f   ); // campo de vision vertical en grados
cam.setNearPlane( 0.1f  );
cam.setFarPlane ( 1000.f );
```

**Movimiento relativo** (tipico para camara FPS):

```cpp
cam.moveForward( speed  * deltaTime );
cam.moveRight  ( strafe * deltaTime );
cam.moveUp     ( vertical * deltaTime );
cam.rotateY    ( mouseX * sensitivity );
cam.rotateX    ( mouseY * sensitivity );
```

La camara expone tambien getters de los vectores locales de orientacion (`getForward()`, `getRight()`, `getUp()`), utiles para implementar logica de movimiento en espacio de camara.

@note El aspect ratio se actualiza automaticamente cuando la ventana cambia de tamano; no es necesario gestionarlo manualmente.

---

## Bucle principal y eventos de ventana

El motor tiene una capa de contrato de eventos desacoplada. Necesita que la aplicacion traduzca los eventos nativos a `WindowEvent` y los entregue al motor.

Actualmente se soporta el evento `Resized`, que provoca la recreacion interna del swapchain y el reajuste del aspect ratio de la camara:

```cpp
while ( running )
{
    SDL_Event ev;
    while ( SDL_PollEvent( &ev ) )
    {
        if ( ev.type == SDL_QUIT ) running = false;

        if ( ev.type == SDL_WINDOWEVENT &&
             ev.window.event == SDL_WINDOWEVENT_RESIZED )
        {
            WindowEvent wev;
            wev.type   = WindowEventType::Resized;
            wev.width  = ev.window.data1;
            wev.height = ev.window.data2;
            engine.handleWindowEvent( wev );
        }
    }

    // Logica de la aplicacion
    entity.rotateY( glm::radians( 30.f ) * deltaTime );

    // Renderizar y presentar el frame actual
    engine.drawFrame();
}
```

`drawFrame()` bloquea si la GPU esta ocupada con los frames en vuelo anteriores (hasta un maximo de `ResourceLimits::MAX_FRAMES_IN_FLIGHT`), ejecuta los pases de renderizado diferido y presenta el resultado en la ventana.

---

## Manejo de errores

El motor usa excepciones tipadas que exponen un codigo de error, permitiendo reaccionar de forma programatica sin depender del texto del mensaje:

```cpp
try {
    scene.destroyEntity( handle );
} catch ( const SceneException& e ) {
    if ( e.code() == SceneErrorCode::StaleHandle ) {
        // La entidad ya fue destruida previamente
    }
}
```

| Excepcion           | Cuando se lanza                                        |
|---------------------|--------------------------------------------------------|
| `SceneException`    | Operaciones invalidas sobre handles de entidad o luz   |
| `ResourceException` | Operaciones invalidas sobre handles de recurso         |
| `std::logic_error`  | Llamadas al motor antes de `init()` o tras `cleanup()` |

Codigos de `SceneErrorCode`: `InvalidHandle`, `StaleHandle`, `LimitExceeded`.

Codigos de `ResourceErrorCode`: `InvalidName`, `DuplicateName`, `LimitExceeded`, `InvalidHandle`, `StaleHandle`, `DependencyInUse`, `LoadFailed`.

---

## Limites de capacidad

Definidos como constantes de compilacion en `ResourceLimits.h`:

| Recurso / elemento     | Limite  |
|------------------------|---------|
| Entidades en escena    | 16 384  |
| Luces en escena        | 16 384  |
| Mallas                 | 4 096   |
| Materiales             | 20 000  |
| Texturas               | 32      |

Superar cualquiera de estos limites lanza una excepcion con codigo `LimitExceeded`.
