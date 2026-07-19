#pragma once

/**
 * @file Geometry.h
 * @brief Descripcion de las principales primitivas de geometria.
 */

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

class Plane {
public:

	Plane() = default;

	//Crea un plano. Se asume que la normal pasada ya está normalizada
	Plane(const glm::vec3& point, const glm::vec3& normal) {
		_normal = glm::normalize(normal);
		_distance = glm::dot(point,normal);
	}

	//Da la distancia con signo de un punto al plano. Si es positivo indica que se encuentra por delante de la normal
	float signedDistanceToPlane(const glm::vec3& point) const {
		return glm::dot(_normal, point) - _distance;
	}

	inline const glm::vec3 getNormal() const { return _normal; }

private:
	//vector normal al plano
	glm::vec3 _normal = glm::vec3(0);
	//distancia del pano al origen del mundo
	float _distance = 0;
};

struct Frustrum { 

	//projection constructor
	Frustrum(glm::vec3 position ,glm::vec3 up, glm::vec3 right, glm::vec3 front, float aspect, float fovY, float zNear, float zFar) {

		const float halfVSide = zFar * tanf(glm::radians(fovY) * .5f);
		const float halfHSide = halfVSide * aspect;
		const glm::vec3 frontMultFar = zFar * front;

		nearFace = { position + zNear * front, front };
		farFace = { position + frontMultFar, -front };
		rightFace = { position, glm::normalize(glm::cross(frontMultFar - right * halfHSide, up)) };
		leftFace = { position, glm::normalize(glm::cross(up, frontMultFar + right * halfHSide)) };
		topFace = { position, glm::normalize(glm::cross(right, frontMultFar - up * halfVSide)) };
		downFace = { position, glm::normalize(glm::cross(frontMultFar + up * halfVSide, right)) };
	}

	//ortho constructor
	Frustrum(glm::vec3 position, glm::vec3 front, float top, float left, float zNear, float zFar) {

		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));

		if (glm::all(glm::isnan(right))) {
			right = glm::vec3(1, 0, 0);
		}

		const glm::vec3 up = glm::normalize(glm::cross(right, front));


		nearFace = Plane( position + zNear * front, front );
		farFace = { position + zFar * front, -front };
		rightFace = {position-right*left,-right};
		leftFace = {position+right * left,right};
		topFace = {position-up*top,-up};
		downFace ={position+up*top,up} ;

	}


	Plane nearFace;
	Plane farFace;
	Plane rightFace;
	Plane leftFace;
	Plane topFace;
	Plane downFace;
};

class Primitive {
public:
	virtual bool isOnFrusturm(const Frustrum& camFrustrum,const glm::mat4& modelMat) const = 0;
	virtual bool isOnOrForwardPlane(const Plane& plane) const = 0;

	bool isOnFrusturmCam(const Frustrum& camFrustrum) const {
		return (
			isOnOrForwardPlane(camFrustrum.nearFace) &&
			isOnOrForwardPlane(camFrustrum.farFace) &&
			isOnOrForwardPlane(camFrustrum.rightFace) &&
			isOnOrForwardPlane(camFrustrum.leftFace) &&
			isOnOrForwardPlane(camFrustrum.topFace) &&
			isOnOrForwardPlane(camFrustrum.downFace)
			);
	}
};


class Sphere : public Primitive {
public:

	Sphere(const glm::vec3& center, float radius) {
		_center = center;
		_radius = radius;
	}

	bool isOnOrForwardPlane(const Plane& plane) const final{
		return plane.signedDistanceToPlane(_center) < -_radius;
	}

	bool isOnFrusturm(const Frustrum& camFrustrum, const glm::mat4& modelMat) const {

		const glm::vec3 globalScale = glm::vec3(glm::length(modelMat[0]), glm::length(modelMat[1]), glm::length(modelMat[2]));

		const glm::vec3 globalCenter = glm::vec3(modelMat * glm::vec4(_center, 1.f));

		const float maxScale = std::max(std::max(globalScale.x, globalScale.y), globalScale.z);

		Sphere transformedSphere ( globalCenter, _radius * (maxScale *0.5));

		return transformedSphere.isOnFrusturmCam(camFrustrum);
	}

private:
	glm::vec3 _center = glm::vec3(0);
	float _radius =0;
};


class AABB : public Primitive {
public:

	AABB(const glm::vec3& min, const glm::vec3& max) {
		this->min = min;
		this->max = max;
		_center = (max + min) * 0.5f;
		_extents = glm::vec3(max.x - _center.x, max.y - _center.y, max.z - _center.z);
	}

	void updateData(const glm::vec3& min, const glm::vec3& max) {
		this->min = min;
		this->max = max;
		_center = (max + min) * 0.5f;
		_extents = glm::vec3(max.x - _center.x, max.y - _center.y, max.z - _center.z);
	}

	glm::vec3 min;
	glm::vec3 max;

	bool isOnOrForwardPlane(const Plane& plane) const final
	{
		// Compute the projection interval radius of b onto L(t) = b.c + t * p.n
		const float r = _extents.x * std::abs(plane.getNormal().x) + _extents.y * std::abs(plane.getNormal().y) +
			_extents.z * std::abs(plane.getNormal().z);

		bool ret = -r <= plane.signedDistanceToPlane(_center);

		return ret;
	}

	bool isOnFrusturm(const Frustrum& camFrustrum, const glm::mat4& modelMat) const {

		const glm::vec3 globalCenter = glm::vec3(modelMat * glm::vec4(_center, 1.f));

		// Scaled orientation
		const glm::vec3 right = glm::vec3(modelMat[0]) * _extents.x;
		const glm::vec3 up = glm::vec3(modelMat[1]) * _extents.y;
		const glm::vec3 forward = glm::vec3(modelMat[2]) * _extents.z;

		//assert(right != glm::vec3(0) && up != glm::vec3(0) != 0 && forward != glm::vec3(0));

		const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

		const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

		const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

		AABB transformedBox(globalCenter, newIi,newIj,newIk);

		return transformedBox.isOnFrusturmCam(camFrustrum);
	}

private:

	AABB(const glm::vec3& center, float i, float j, float k) {
		_center = center;
		_extents = glm::vec3(i, j, k);
	}

	glm::vec3 _center = glm::vec3(0);
	glm::vec3 _extents = glm::vec3(0);
};


