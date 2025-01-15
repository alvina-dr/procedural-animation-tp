#pragma once

#include <glm/ext/scalar_constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

struct Camera {
	float fov;
	float radius;
	float theta;
	float phi;
	glm::vec3 o;
	glm::vec3 eye;
	glm::vec3 up;
};

inline void cameraCompute(Camera& c) {
	c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;
	c.eye.y = cos(c.phi) * c.radius + c.o.y;
	c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;
	c.up = glm::vec3(0.f, c.phi < glm::pi<float>() ? 1.f : -1.f, 0.f);
}

inline void cameraCreate(Camera& c) {
	c.fov = glm::radians(45.f);
	c.phi = glm::pi<float>() / 3.f;
	c.theta = glm::pi<float>() / 3.f;
	c.radius = 5.f;
	c.o.x = 0.f;
	c.o.y = 0.f;
	c.o.z = 0.f;
	cameraCompute(c);
}

inline void cameraZoom(Camera& c, float factor) {
	c.radius += factor * c.radius;
	if (c.radius < 0.1f) {
		c.radius = 10.f;
		c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
	}
	cameraCompute(c);
}

inline void cameraTurn(Camera& c, float phi, float theta) {
	c.theta += 1.f * theta;
	c.phi -= 1.f * phi;
	if (c.phi >= (2 * glm::pi<float>()) - 0.1f)
		c.phi = 0.00001f;
	else if (c.phi <= 0)
		c.phi = 2 * glm::pi<float>() - 0.1f;
	cameraCompute(c);
}

inline void cameraPan(Camera& c, float x, float y) {
	glm::vec3 up(0.f, c.phi < glm::pi<float>() ? 1.f : -1.f, 0.f);
	glm::vec3 fwd = glm::normalize(c.o - c.eye);
	glm::vec3 side = glm::normalize(glm::cross(fwd, up));
	c.up = glm::normalize(glm::cross(side, fwd));
	c.o[0] += up[0] * y * c.radius * 2;
	c.o[1] += up[1] * y * c.radius * 2;
	c.o[2] += up[2] * y * c.radius * 2;
	c.o[0] -= side[0] * x * c.radius * 2;
	c.o[1] -= side[1] * x * c.radius * 2;
	c.o[2] -= side[2] * x * c.radius * 2;
	cameraCompute(c);
}