#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glad.h>

struct Buffer3D {
	enum {
		BufferAttribVertex = 0,
		BufferAttribNormal,
		BufferAttribColor,
		BufferAttribCount
	};
	GLuint vao = 0;
	GLuint vbos[BufferAttribCount] = {};
	GLuint ibo = 0;
	GLsizei vertexCount = 0;
	GLsizei indexCount = 0;
};

struct CreateBuffer3DParams {
	glm::vec3 const* pVertices = nullptr;
	glm::vec3 const* pNormals = nullptr;
	glm::vec4 const* pColors = nullptr;
	unsigned int const* pIndices = nullptr;
	GLsizei vertexCount = 0;
	GLsizei indexCount = 0;
};

void createBuffer3D(Buffer3D& buffer, const CreateBuffer3DParams& params);

void deleteBuffer3D(Buffer3D& buffer);

struct Buffer2D {
	enum {
		BufferAttribVertex = 0,
		BufferAttribColor,
		BufferAttribCount
	};
	GLuint vao = 0;
	GLuint vbos[BufferAttribCount] = {};
	GLsizei vertexCount = 0;
};

struct CreateBuffer2DParams {
	glm::vec2 const* pVertices = nullptr;
	glm::vec4 const* pColors = nullptr;
	GLsizei vertexCount = 0;
};

void createBuffer2D(Buffer2D& buffer, const CreateBuffer2DParams& params);

void deleteBuffer2D(Buffer2D& buffer);
