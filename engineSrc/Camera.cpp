#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position,
    glm::vec3 rotationRadians,
    float     fov,
    float     aspectRatio,
    float     nearPlane,
    float     farPlane)
    : _position(position)
    , _rotation(rotationRadians)
    , _fov(fov)
    , _aspectRatio(aspectRatio)
    , _nearPlane(nearPlane)
    , _farPlane(farPlane)
{}

glm::vec3 Camera::getForward() const
{
    // Reconstruimos la direccion a partir de la rotacion Euler XYZ.
    // pitch = _rotation.x, yaw = _rotation.y, roll = _rotation.z (ignorado para forward).
    glm::vec3 forward;
    forward.x = std::cos(_rotation.x) * std::sin(-_rotation.y);  // negado
    forward.y = -std::sin(_rotation.x);
    forward.z = std::cos(_rotation.x) * std::cos(-_rotation.y);  // negado
    return glm::normalize(forward);
}

glm::vec3 Camera::getRight() const
{
    return glm::normalize(glm::cross(getForward(), _worldUp));
}

glm::vec3 Camera::getUp() const
{
    return glm::normalize(glm::cross(getRight(), getForward()));
}

glm::mat4 Camera::getViewMatrix() const
{
    const glm::vec3 forward = getForward();
    return glm::lookAt(_position, _position + forward, _worldUp);
}

glm::mat4 Camera::getProjMatrix() const
{
    return glm::perspective(glm::radians(_fov), _aspectRatio, _nearPlane, _farPlane);
}

void Camera::moveForward(float distance)
{
    _position += getForward() * distance;
}

void Camera::moveRight(float distance)
{
    _position += getRight() * distance;
}

void Camera::moveUp(float distance)
{
    _position += _worldUp * distance;
}