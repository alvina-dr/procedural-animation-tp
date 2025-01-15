#pragma once

#include <glad.h>
#include <glm/fwd.hpp>

struct Buffer3D;
struct Buffer2D;
struct RenderEngine;
struct ShaderProgram3D;

enum class eDrawMode : GLenum {
	Triangles = GL_TRIANGLES,
	Lines = GL_LINES,
	Points = GL_POINTS,
};

struct RenderApi3D {
	RenderEngine const* pRenderEngine;
	ShaderProgram3D const* pShader3D;

	void buffer(const Buffer3D& buffer, eDrawMode drawMode, glm::mat4 const* pModel) const;

	// warning: if you want to draw A-B-C-D, then vertices should contain A-B-B-C-C-D 
	void lines(glm::vec3 const* vertices, unsigned int vertexCount, const glm::vec4& color, glm::mat4 const* pModel) const;

	void grid(float size, unsigned int subdivisions, const glm::vec4& color, glm::mat4 const* pModel) const;

	void axisXYZ(glm::mat4 const* pModel) const;

	void solidCube(float size, const glm::vec4& color, glm::mat4 const* pModel) const;

	void solidSphere(const glm::vec3& center, float radius, unsigned int horizontalSubdivisions, unsigned int verticalSubdivisions, const glm::vec4& color) const;

	void bone(const glm::vec3& childRelativePosition, const glm::vec4& color, const glm::quat& parentAbsoluteRotation, const glm::vec3& parentAbsolutePosition) const;
	
	void horizontalPlane(const glm::vec3& center, const glm::vec2& size, unsigned int SideSubdivision, const glm::vec4& color) const;
};

struct RenderApi2D {
	RenderEngine const* pRenderEngine;

	void buffer(const Buffer2D& buffer, eDrawMode drawMode) const;

	// warning: if you want to draw A-B-C-D, then vertices should contain A-B-B-C-C-D 
	void lines(glm::vec2 const* vertices, unsigned int vertexCount, const glm::vec4& color) const;

	void quadFill(const glm::vec2& min, const glm::vec2& max, const glm::vec4& color) const;
	void quadContour(const glm::vec2& min, const glm::vec2& max, const glm::vec4& color) const;

	void circleFill(const glm::vec2& center, float radius, unsigned int subdivisions, const glm::vec4& color) const;
	void circleContour(const glm::vec2& center, float radius, unsigned int subdivisions, const glm::vec4& color) const;

	void arrow(const glm::vec2& from, const glm::vec2& to, float thickness, float hatRatio /*between 0 and 1*/, const glm::vec4& color) const;
};