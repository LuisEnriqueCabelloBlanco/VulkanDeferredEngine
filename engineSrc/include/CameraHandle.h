#pragma once

/**
 * @file CameraHandle.h
 * @brief Interfaz publica de la camara principal de una Scene.
 */

#include <glm/glm.hpp>

class Camera;
class Scene;

// =============================================================================
// CameraHandle
// =============================================================================

/**
 * @brief Interfaz publica de la camara principal para el codigo de aplicacion.
 *
 * Scene posee la Camera y emite un CameraHandle unico accesible mediante
 * Scene::getCamera(). El handle no tiene estado propio de transformacion:
 * todas las operaciones delegan directamente en la Camera subyacente, que es
 * la unica fuente de verdad.
 *
 * El handle es non-copyable y non-movable: existe exactamente uno por Scene
 * y su ciclo de vida esta ligado al de la propia Scene.
 *
 * Ejemplo de uso tipico:
 * @code
 * CameraHandle& cam = scene.getCamera();
 * cam.setPosition( { 0.f, 2.f, -5.f } );
 * cam.setFOV( 75.f );
 * cam.moveForward( delta * speed );
 * @endcode
 *
 * @see Scene::getCamera()
 */
class CameraHandle {
public:
    CameraHandle( const CameraHandle& )            = delete;
    CameraHandle& operator=( const CameraHandle& ) = delete;
    CameraHandle( CameraHandle&& )                 = delete;
    CameraHandle& operator=( CameraHandle&& )      = delete;

    ~CameraHandle() = default;

    // -------------------------------------------------------------------------
    // Posicion y orientacion — setters absolutos
    // -------------------------------------------------------------------------

    /**
     * @brief Establece la posicion absoluta de la camara en el espacio del mundo.
     * @param position Nueva posicion.
     */
    void setPosition( const glm::vec3& position );

    /**
     * @brief Establece la rotacion absoluta de la camara en radianes, orden XYZ.
     * @param rotationRadians Rotacion en radianes (pitch X, yaw Y, roll Z).
     */
    void setRotation( const glm::vec3& rotationRadians );

    /**
     * @brief Establece solo el yaw (rotacion en Y). Util para camaras tipo FPS.
     * @param radians Angulo en radianes.
     */
    void setYaw  ( float radians );

    /**
     * @brief Establece solo el pitch (rotacion en X).
     * @param radians Angulo en radianes.
     */
    void setPitch( float radians );

    // -------------------------------------------------------------------------
    // Posicion y orientacion — mutaciones relativas
    // -------------------------------------------------------------------------

    /**
     * @brief Desplaza la camara sumando un vector delta en espacio del mundo.
     * @param delta Desplazamiento a aplicar.
     */
    void translate  ( const glm::vec3& delta );

    /**
     * @brief Rota la camara incrementalmente en radianes, orden XYZ.
     * @param deltaRadians Incremento de rotacion (pitch, yaw, roll).
     */
    void rotate     ( const glm::vec3& deltaRadians );

    /**
     * @brief Rota la camara alrededor del eje X (pitch).
     * @param radians Incremento en radianes.
     */
    void rotateX    ( float radians );

    /**
     * @brief Rota la camara alrededor del eje Y (yaw).
     * @param radians Incremento en radianes.
     */
    void rotateY    ( float radians );

    /**
     * @brief Rota la camara alrededor del eje Z (roll).
     * @param radians Incremento en radianes.
     */
    void rotateZ    ( float radians );

    /**
     * @brief Mueve la camara hacia adelante en su eje local.
     * @param distance Distancia a recorrer (negativo = hacia atras).
     */
    void moveForward( float distance );

    /**
     * @brief Desplaza la camara lateralmente (strafe) en su eje local derecho.
     * @param distance Distancia a recorrer (negativo = hacia la izquierda).
     */
    void moveRight  ( float distance );

    /**
     * @brief Mueve la camara verticalmente en el eje Y global.
     * @param distance Distancia a recorrer (negativo = hacia abajo).
     */
    void moveUp     ( float distance );

    // -------------------------------------------------------------------------
    // Parametros de proyeccion
    // -------------------------------------------------------------------------

    /**
     * @brief Establece el campo de vision vertical de la proyeccion perspectiva.
     * @param degrees FOV en grados.
     */
    void setFOV        ( float degrees   );

    /**
     * @brief Establece la distancia del plano cercano de recorte.
     * @param nearPlane Distancia del near plane (debe ser mayor que 0).
     */
    void setNearPlane  ( float nearPlane );

    /**
     * @brief Establece la distancia del plano lejano de recorte.
     * @param farPlane Distancia del far plane (debe ser mayor que nearPlane).
     */
    void setFarPlane   ( float farPlane  );

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    /// @brief Devuelve la posicion actual de la camara en el espacio del mundo.
    glm::vec3 getPosition()    const;

    /// @brief Devuelve la rotacion actual de la camara en radianes, orden XYZ.
    glm::vec3 getRotation()    const;

    /// @brief Devuelve el vector unitario que apunta hacia adelante desde la camara.
    glm::vec3 getForward()     const;

    /// @brief Devuelve el vector unitario que apunta hacia la derecha desde la camara.
    glm::vec3 getRight()       const;

    /// @brief Devuelve el vector unitario que apunta hacia arriba desde la camara.
    glm::vec3 getUp()          const;

    /// @brief Devuelve el campo de vision vertical en grados.
    float     getFOV()         const;

    /// @brief Devuelve la distancia del plano cercano de recorte.
    float     getNearPlane()   const;

    /// @brief Devuelve la distancia del plano lejano de recorte.
    float     getFarPlane()    const;

    /// @brief Devuelve la razon de aspecto actual (ancho / alto), gestionada por RenderEngine.
    float     getAspectRatio() const;

private:
    /// Solo Scene puede construir un CameraHandle.
    friend class Scene;

    explicit CameraHandle( Camera& camera ) : _camera( camera ) {}

    Camera& _camera;
};
