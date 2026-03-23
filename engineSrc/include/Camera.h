#pragma once
#include <glm/glm.hpp>


class Camera
{
public:
	Camera();

	Camera( glm::vec3 pos, glm::vec3 dir, glm::vec3 up, float fov, float aspectRatio, float nearDistance, float farDistance );

	~Camera();


	glm::mat4 getProjMatrix();
	glm::mat4 getViewMatrix();

	inline void setPosition( glm::vec3 pos ) {
		_position = pos;
	}

	inline glm::vec3 getDirection() { return _direction; }

	void rotateY( float degrees );

	glm::vec3 getPos() { return _position; }
private:
	glm::vec3 _position = glm::vec3(0);
	glm::vec3 _direction = glm::vec3(1,0,0);
	glm::vec3 _up = glm::vec3(0,1,0);

	float _fov = 90.f;
	float _nearPlane = 0.1f;
	float _farPlane = 10.0f;
	float _aspectRatio = 1; //te quiero mucho enrique :3 

};

