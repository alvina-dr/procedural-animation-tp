#include "drawbuffer.h"
#include <glad.h>

void createBuffer3D(Buffer3D& buffer, const CreateBuffer3DParams& params) {
	assert(buffer.vao == 0); // trying to create a buffer already initialized

	glGenVertexArrays(1, &buffer.vao);

	glBindVertexArray(buffer.vao);

	// Bind vertices and upload data
	assert(buffer.vbos[Buffer3D::BufferAttribVertex] == 0); // trying to create a buffer already initialized
	glGenBuffers(1, &buffer.vbos[Buffer3D::BufferAttribVertex]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.vbos[Buffer3D::BufferAttribVertex]);
	glEnableVertexAttribArray(0);
	{
		constexpr size_t size = sizeof(*params.pVertices) / sizeof((*params.pVertices)[0]);
		constexpr size_t stride = sizeof(*params.pVertices);
		glVertexAttribPointer(0, size, GL_FLOAT, GL_FALSE, stride, (void*)0);
	}
	glBufferData(GL_ARRAY_BUFFER, params.vertexCount * sizeof(*params.pVertices), params.pVertices, GL_STATIC_DRAW);

	assert(buffer.vbos[Buffer3D::BufferAttribNormal] == 0); // trying to create a buffer already initialized
	if(params.pNormals) {
		glGenBuffers(1, &buffer.vbos[Buffer3D::BufferAttribNormal]);
		// Bind normals and upload data
		glBindBuffer(GL_ARRAY_BUFFER, buffer.vbos[buffer.BufferAttribNormal]);
		glEnableVertexAttribArray(1);
		{
			constexpr size_t size = sizeof(*params.pNormals) / sizeof((*params.pNormals)[0]);
			constexpr size_t stride = sizeof(*params.pNormals);
			glVertexAttribPointer(1, size, GL_FLOAT, GL_FALSE, stride, (void*)0);
		}
		glBufferData(GL_ARRAY_BUFFER, params.vertexCount * sizeof(*params.pNormals), params.pNormals, GL_STATIC_DRAW);
	} else {
		buffer.vbos[Buffer3D::BufferAttribNormal] = 0;
	}

	// Bind colors and upload data
	assert(buffer.vbos[Buffer3D::BufferAttribColor] == 0); // trying to create a buffer already initialized
	glGenBuffers(1, &buffer.vbos[Buffer3D::BufferAttribColor]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.vbos[buffer.BufferAttribColor]);
	glEnableVertexAttribArray(2);
	{
		constexpr size_t size = sizeof(*params.pColors) / sizeof((*params.pColors)[0]);
		constexpr size_t stride = sizeof(*params.pColors);
		glVertexAttribPointer(2, size, GL_FLOAT, GL_FALSE, stride, (void*)0);
	}
	glBufferData(GL_ARRAY_BUFFER, params.vertexCount * sizeof(*params.pColors), params.pColors, GL_STATIC_DRAW);

	if(params.pIndices) {
		glGenBuffers(1, &buffer.ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*params.pIndices) * params.indexCount, params.pIndices, GL_STATIC_DRAW);
	} else {
		buffer.ibo = 0;
		buffer.indexCount = 0;
	}

	buffer.vertexCount = params.vertexCount;
	buffer.indexCount = params.indexCount;

	// Unbind everything. Potentially illegal on some implementations
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void deleteBuffer3D(Buffer3D& buffer) {
	glDeleteBuffers(buffer.BufferAttribCount, buffer.vbos);
	glDeleteBuffers(1, &buffer.ibo);
	glDeleteVertexArrays(1, &buffer.vao);
	memset(buffer.vbos, 0, sizeof(buffer.vbos));
	buffer.vao = 0;
}

void createBuffer2D(Buffer2D& buffer, const CreateBuffer2DParams& params) {
	glGenVertexArrays(1, &buffer.vao);
	glGenBuffers(buffer.BufferAttribCount, buffer.vbos);

	glBindVertexArray(buffer.vao);

	// Bind vertices and upload data
	glBindBuffer(GL_ARRAY_BUFFER, buffer.vbos[buffer.BufferAttribVertex]);
	glEnableVertexAttribArray(0);
	{
		constexpr size_t size = sizeof(*params.pVertices) / sizeof((*params.pVertices)[0]);
		constexpr size_t stride = sizeof(*params.pVertices);
		glVertexAttribPointer(0, size, GL_FLOAT, GL_FALSE, stride, (void*)0);
	}
	glBufferData(GL_ARRAY_BUFFER, params.vertexCount * sizeof(*params.pVertices), params.pVertices, GL_STATIC_DRAW);

	// Bind colors and upload data
	glBindBuffer(GL_ARRAY_BUFFER, buffer.vbos[buffer.BufferAttribColor]);
	glEnableVertexAttribArray(1);
	{
		constexpr size_t size = sizeof(*params.pColors) / sizeof((*params.pColors)[0]);
		constexpr size_t stride = sizeof(*params.pColors);
		glVertexAttribPointer(1, size, GL_FLOAT, GL_FALSE, stride, (void*)0);
	}
	glBufferData(GL_ARRAY_BUFFER, params.vertexCount * sizeof(*params.pColors), params.pColors, GL_STATIC_DRAW);

	buffer.vertexCount = params.vertexCount;

	// Unbind everything. Potentially illegal on some implementations
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void deleteBuffer2D(Buffer2D& buffer) {
	glDeleteBuffers(buffer.BufferAttribCount, buffer.vbos);
	glDeleteVertexArrays(1, &buffer.vao);
	memset(buffer.vbos, 0, sizeof(buffer.vbos));
	buffer.vao = 0;
}
