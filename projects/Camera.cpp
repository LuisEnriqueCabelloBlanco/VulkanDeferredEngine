#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_clip_space.hpp>


Camera::Camera()
{
}

Camera::Camera( glm::vec3 pos, glm::vec3 dir, glm::vec3 up, float fov, float aspectRatio, float nearDistance, float farDistance ) {
    _position = pos;
    _direction = dir;
    _up = up;

    _fov = fov;
    _aspectRatio = aspectRatio;
    _nearPlane = nearDistance;
    _farPlane = farDistance;
}


Camera::~Camera()
{
}

glm::mat4 Camera::getProjMatrix()
{
    return glm::perspective( glm::radians( _fov ), _aspectRatio, _nearPlane, _farPlane );
}

glm::mat4 Camera::getViewMatrix()
{
    
    return glm::lookAt( _position, _position + _direction, _up );
}
