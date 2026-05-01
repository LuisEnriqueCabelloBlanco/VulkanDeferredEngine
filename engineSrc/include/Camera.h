#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraHandle;

// =============================================================================
// Camera
// =============================================================================
/*
  Datos y logica de la camara principal de la escena.

  Camera es propiedad de Scene y no se expone directamente al codigo de
  aplicacion. La aplicacion interactua con ella a traves de CameraHandle,
  que actua como interfaz rica sobre este objeto.

  El Transform (posicion + rotacion) vive dentro de Camera como fuente de
  verdad unica. CameraHandle delega todas las mutaciones aqui directamente,
  sin mantener estado propio de transformacion.

  CONVENCION DE ROTACION
  ----------------------
  Euler XYZ en radianes (pitch, yaw, roll). La direccion de vision se
  recalcula a partir de la rotacion cada vez que se necesita la view matrix.

  MATRICES
  --------
  getProjMatrix() y getViewMatrix() recalculan siempre. No hay cache interna
  porque son llamadas una vez por frame desde RenderEngine.
*/
class Camera {
public:
    Camera() = default;

    Camera(glm::vec3 position,
        glm::vec3 rotationRadians,
        float     fov,
        float     aspectRatio,
        float     nearPlane,
        float     farPlane);

    ~Camera() = default;

    glm::mat4 getProjMatrix() const;
    glm::mat4 getViewMatrix() const;

    // Devuelve la direccion de vision actual (calculada a partir de la rotacion).
    glm::vec3 getForward() const;

    // Devuelve el vector up actual (calculado a partir de la rotacion).
    glm::vec3 getUp() const;

    // Devuelve el vector right actual.
    glm::vec3 getRight() const;

    glm::vec3 getPosition()   const { return _position; }
    glm::vec3 getRotation()   const { return _rotation; }
    float     getFOV()        const { return _fov; }
    float     getNearPlane()  const { return _nearPlane; }
    float     getFarPlane()   const { return _farPlane; }
    float     getAspectRatio() const { return _aspectRatio; }

private:
    // Solo Scene y RenderEngine acceden a los setters directamente.
    // La aplicacion los usa a traves de CameraHandle.
    friend class Scene;
    friend class CameraHandle;
    friend class RenderEngine;

    // --- Posicion y orientacion ---

    void setPosition(const glm::vec3& position) { _position = position; }
    void setRotation(const glm::vec3& rotationRadians) { _rotation = rotationRadians; }

    void translate(const glm::vec3& delta) { _position += delta; }
    void rotate(const glm::vec3& deltaRadians) { _rotation += deltaRadians; }
    void rotateX(float radians) { _rotation.x += radians; }
    void rotateY(float radians) { _rotation.y += radians; }
    void rotateZ(float radians) { _rotation.z += radians; }

    // Mueve la camara a lo largo de su eje forward local.
    void moveForward(float distance);

    // Mueve la camara a lo largo de su eje right local (strafe).
    void moveRight(float distance);

    // Mueve la camara a lo largo del eje Y global.
    void moveUp(float distance);

    // --- Parametros de proyeccion ---

    void setFOV(float degrees) { _fov = degrees; }
    void setNearPlane(float nearPlane) { _nearPlane = nearPlane; }
    void setFarPlane(float farPlane) { _farPlane = farPlane; }
    void setAspectRatio(float aspectRatio) { _aspectRatio = aspectRatio; }

    // --- Datos ---

    glm::vec3 _position = glm::vec3(0.0f);
    glm::vec3 _rotation = glm::vec3(0.0f);  // radianes, orden XYZ (pitch, yaw, roll)
    glm::vec3 _worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float _fov = 90.0f;
    float _nearPlane = 0.1f;
    float _farPlane = 100.0f;
    float _aspectRatio = 1.0f;
};