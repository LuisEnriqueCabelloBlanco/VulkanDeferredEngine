#pragma once

#include <glm/glm.hpp>

class Camera;
class Scene;

// =============================================================================
// CameraHandle
// =============================================================================
/*
  Interfaz publica de la camara para el codigo de aplicacion.

  Scene posee la Camera y emite un CameraHandle unico. El handle no tiene
  estado propio de transformacion: todas las operaciones delegan directamente
  en la Camera subyacente, que es la unica fuente de verdad.

  El handle es non-copyable y non-movable: hay exactamente uno, obtenido
  con Scene::getCamera(), y su ciclo de vida esta ligado al de Scene.

  Uso tipico:
      CameraHandle& cam = scene.getCamera();
      cam.setPosition( { 0.f, 2.f, -5.f } );
      cam.setFOV( 75.f );
      cam.moveForward( delta * speed );
*/
class CameraHandle {
public:
    CameraHandle( const CameraHandle& ) = delete;
    CameraHandle& operator=( const CameraHandle& ) = delete;
    CameraHandle( CameraHandle&& ) = delete;
    CameraHandle& operator=( CameraHandle&& ) = delete;

    ~CameraHandle() = default;

    // -------------------------------------------------------------------------
    // Posicion y orientacion — setters absolutos
    // -------------------------------------------------------------------------

    void setPosition( const glm::vec3& position );

    // Rotacion en radianes, orden XYZ (pitch, yaw, roll).
    void setRotation( const glm::vec3& rotationRadians );

    // Shortcut: solo actualiza yaw (rotacion en Y). Util para FPS.
    void setYaw  ( float radians );
    void setPitch( float radians );

    // -------------------------------------------------------------------------
    // Posicion y orientacion — mutaciones relativas
    // -------------------------------------------------------------------------

    void translate  ( const glm::vec3& delta );
    void rotate     ( const glm::vec3& deltaRadians );
    void rotateX    ( float radians );   // pitch
    void rotateY    ( float radians );   // yaw
    void rotateZ    ( float radians );   // roll

    // Movimiento en ejes locales de la camara.
    void moveForward( float distance );
    void moveRight  ( float distance );  // strafe
    void moveUp     ( float distance );  // sube/baja en eje Y global

    // -------------------------------------------------------------------------
    // Parametros de proyeccion
    // -------------------------------------------------------------------------

    // fov en grados.
    void setFOV        ( float degrees   );
    void setNearPlane  ( float nearPlane );
    void setFarPlane   ( float farPlane  );

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    glm::vec3 getPosition()    const;
    glm::vec3 getRotation()    const;   // radianes XYZ

    // Vectores de orientacion calculados a partir de la rotacion actual.
    glm::vec3 getForward()     const;
    glm::vec3 getRight()       const;
    glm::vec3 getUp()          const;

    float     getFOV()         const;
    float     getNearPlane()   const;
    float     getFarPlane()    const;
    float     getAspectRatio() const;

private:
    // Solo Scene puede construir un CameraHandle.
    friend class Scene;

    explicit CameraHandle( Camera& camera ) : _camera( camera ) {}

    Camera& _camera;
};
