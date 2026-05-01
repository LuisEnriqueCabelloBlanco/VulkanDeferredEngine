#include "CameraHandle.h"
#include "Camera.h"

// -------------------------------------------------------------------------
// Posicion y orientacion — setters absolutos
// -------------------------------------------------------------------------

void CameraHandle::setPosition( const glm::vec3& position )
{
    _camera.setPosition( position );
}

void CameraHandle::setRotation( const glm::vec3& rotationRadians )
{
    _camera.setRotation( rotationRadians );
}

void CameraHandle::setYaw( float radians )
{
    glm::vec3 rot = _camera.getRotation();
    rot.y = radians;
    _camera.setRotation( rot );
}

void CameraHandle::setPitch( float radians )
{
    glm::vec3 rot = _camera.getRotation();
    rot.x = radians;
    _camera.setRotation( rot );
}

// -------------------------------------------------------------------------
// Posicion y orientacion — mutaciones relativas
// -------------------------------------------------------------------------

void CameraHandle::translate( const glm::vec3& delta )
{
    _camera.translate( delta );
}

void CameraHandle::rotate( const glm::vec3& deltaRadians )
{
    _camera.rotate( deltaRadians );
}

void CameraHandle::rotateX( float radians )
{
    _camera.rotateX( radians );
}

void CameraHandle::rotateY( float radians )
{
    _camera.rotateY( radians );
}

void CameraHandle::rotateZ( float radians )
{
    _camera.rotateZ( radians );
}

void CameraHandle::moveForward( float distance )
{
    _camera.moveForward( distance );
}

void CameraHandle::moveRight( float distance )
{
    _camera.moveRight( distance );
}

void CameraHandle::moveUp( float distance )
{
    _camera.moveUp( distance );
}

// -------------------------------------------------------------------------
// Parametros de proyeccion
// -------------------------------------------------------------------------

void CameraHandle::setFOV( float degrees )
{
    _camera.setFOV( degrees );
}

void CameraHandle::setNearPlane( float nearPlane )
{
    _camera.setNearPlane( nearPlane );
}

void CameraHandle::setFarPlane( float farPlane )
{
    _camera.setFarPlane( farPlane );
}

// -------------------------------------------------------------------------
// Getters
// -------------------------------------------------------------------------

glm::vec3 CameraHandle::getPosition()    const { return _camera.getPosition(); }
glm::vec3 CameraHandle::getRotation()    const { return _camera.getRotation(); }
glm::vec3 CameraHandle::getForward()     const { return _camera.getForward(); }
glm::vec3 CameraHandle::getRight()       const { return _camera.getRight(); }
glm::vec3 CameraHandle::getUp()          const { return _camera.getUp(); }
float     CameraHandle::getFOV()         const { return _camera.getFOV(); }
float     CameraHandle::getNearPlane()   const { return _camera.getNearPlane(); }
float     CameraHandle::getFarPlane()    const { return _camera.getFarPlane(); }
float     CameraHandle::getAspectRatio() const { return _camera.getAspectRatio(); }
