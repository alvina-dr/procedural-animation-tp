#include "renderapi.h"
#include "renderengine.h"
#include "drawbuffer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp>

#define COUNTOF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

namespace {
	struct DUMMY_STACKED_ALLOCATOR {
		enum { MAX_SIZE = 16 * 1024 * 1024};
		char* pCurrent;
		int SizeInUse;
		DUMMY_STACKED_ALLOCATOR() {
			pCurrent = new char[MAX_SIZE];
		}
		void* Allocate(int Size) {
			assert(SizeInUse + Size < MAX_SIZE);
			char* pToReturn = pCurrent;
			pCurrent += Size + 4;
			*(int*)pToReturn = Size + 4;
			SizeInUse += Size + 4;
			return (void*)(pToReturn + 4);
		}
		void Free(void* pPointer) {
			assert(pPointer < pCurrent);
			int* pSize = (int*)pPointer;
			--pSize;
			SizeInUse -= *pSize;
			pCurrent = (char*)pSize;
		}
	};
	DUMMY_STACKED_ALLOCATOR Allocator;
}

void RenderApi3D::buffer(const Buffer3D& buffer, eDrawMode drawMode, glm::mat4 const* pModel) const {
	glm::mat4 model = pModel ? *pModel : glm::identity<glm::mat4>();
	glProgramUniformMatrix4fv(pShader3D->programId, pShader3D->modelLocation, 1, 0, glm::value_ptr(model));

	const bool lightingEnabled = buffer.vbos[Buffer3D::BufferAttribNormal] != 0;
	glProgramUniform1i(pShader3D->programId, pShader3D->lightingEnabledLocation, lightingEnabled);

	assert(buffer.vao); // did you call createDrawBuffer3D ?
	glBindVertexArray(buffer.vao);
	if (buffer.ibo != 0) {
		glDrawElements((GLenum)drawMode, buffer.indexCount, GL_UNSIGNED_INT, nullptr);
	}
	else {
		glDrawArrays((GLenum)drawMode, 0, buffer.vertexCount);
	}
	glBindVertexArray(0);
}

void RenderApi3D::lines(glm::vec3 const* vertices, unsigned int vertexCount, const glm::vec4& color, glm::mat4 const* pModel) const {
	glm::vec4* colors = (glm::vec4*)Allocator.Allocate(sizeof(glm::vec4) * vertexCount);
	for (unsigned int i = 0; i < vertexCount; ++i) {
		colors[i] = color;
	}

	Buffer3D buffer3D;

	CreateBuffer3DParams createCubeBufferParams;
	createCubeBufferParams.pVertices = vertices;
	createCubeBufferParams.pNormals = nullptr;
	createCubeBufferParams.pColors = colors;
	createCubeBufferParams.vertexCount = vertexCount;
	createBuffer3D(buffer3D, createCubeBufferParams);

	buffer(buffer3D, eDrawMode::Lines, pModel);

	deleteBuffer3D(buffer3D);

	Allocator.Free(colors);
}

void RenderApi3D::grid(float size, unsigned int subdivisions, const glm::vec4& color, glm::mat4 const* pModel) const {
	subdivisions = glm::max(subdivisions, 1u);

	const unsigned int lineCount = 4 + 2 * (subdivisions - 1);
	const unsigned int vertexCount = 2 * lineCount;
	glm::vec3* vertices = (glm::vec3*)Allocator.Allocate(sizeof(glm::vec3) * vertexCount);
	glm::vec4* colors = (glm::vec4*)Allocator.Allocate(sizeof(glm::vec4) * vertexCount);

	const float halfSize = 0.5f * size;

	int iVertex = 0;

	vertices[iVertex++] = glm::vec3(-halfSize, 0.f, halfSize);
	vertices[iVertex++] = glm::vec3(halfSize, 0.f, halfSize);
	vertices[iVertex++] = glm::vec3(-halfSize, 0.f, -halfSize);
	vertices[iVertex++] = glm::vec3(halfSize, 0.f, -halfSize);
	vertices[iVertex++] = glm::vec3(-halfSize, 0.f, halfSize);
	vertices[iVertex++] = glm::vec3(-halfSize, 0.f, -halfSize);
	vertices[iVertex++] = glm::vec3(halfSize, 0.f, halfSize);
	vertices[iVertex++] = glm::vec3(halfSize, 0.f, -halfSize);

	for (unsigned int i = 1; i < subdivisions; ++i) {
		const float coord = -halfSize + size * (i / float(subdivisions));
		vertices[iVertex++] = glm::vec3(coord, 0.f, -halfSize);
		vertices[iVertex++] = glm::vec3(coord, 0.f, halfSize);
		vertices[iVertex++] = glm::vec3(-halfSize, 0.f, coord);
		vertices[iVertex++] = glm::vec3(halfSize, 0.f, coord);
	}

	for (unsigned int i = 0; i < vertexCount; ++i) {
		colors[i] = color;
	}

	Buffer3D buffer3D;

	CreateBuffer3DParams createCubeBufferParams;
	createCubeBufferParams.pVertices = vertices;
	createCubeBufferParams.pNormals = nullptr;
	createCubeBufferParams.pColors = colors;
	createCubeBufferParams.vertexCount = vertexCount;
	createBuffer3D(buffer3D, createCubeBufferParams);

	buffer(buffer3D, eDrawMode::Lines, pModel);

	deleteBuffer3D(buffer3D);

	Allocator.Free(colors);
	Allocator.Free(vertices);
}

void RenderApi3D::axisXYZ(glm::mat4 const* pModel) const {

	glm::vec3 vertices[] = {
		glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(1.f, 0.f, 0.f),
		glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f),
		glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 0.f, 1.f),
	};
	constexpr size_t vertexCount = COUNTOF(vertices);

	glm::vec4 colors[] = {
		glm::vec4(1.f, 0.f, 0.f, 1.f),
		glm::vec4(1.f, 0.f, 0.f, 1.f),
		glm::vec4(0.f, 1.f, 0.f, 1.f),
		glm::vec4(0.f, 1.f, 0.f, 1.f),
		glm::vec4(0.f, 0.f, 1.f, 1.f),
		glm::vec4(0.f, 0.f, 1.f, 1.f),
	};

	Buffer3D buffer3D;

	CreateBuffer3DParams createCubeBufferParams;
	createCubeBufferParams.pVertices = vertices;
	createCubeBufferParams.pNormals = nullptr;
	createCubeBufferParams.pColors = colors;
	createCubeBufferParams.vertexCount = vertexCount;
	createBuffer3D(buffer3D, createCubeBufferParams);

	buffer(buffer3D, eDrawMode::Lines, pModel);

	deleteBuffer3D(buffer3D);
}

void RenderApi3D::solidCube(float size, const glm::vec4& color, glm::mat4 const* pModel) const {
	const float halfsize = size * 0.5f;
	glm::vec3 edges[8] =
	{
		{ -halfsize, -halfsize, -halfsize},
		{ +halfsize, -halfsize, -halfsize},
		{ +halfsize, +halfsize, -halfsize},
		{ -halfsize, +halfsize, -halfsize},
		{ -halfsize, -halfsize, +halfsize},
		{ +halfsize, -halfsize, +halfsize},
		{ +halfsize, +halfsize, +halfsize},
		{ -halfsize, +halfsize, +halfsize},
	};

	glm::vec3 faceNormals[6] =
	{
		{ 0, 0, -1 },
		{ +1, 0, 0 },
		{ 0, 0, +1 },
		{ -1, 0, 0 },
		{ 0, +1, 0 },
		{ 0, -1, 0 },
	};

	int indices[36] =
	{
		0, 1, 3, 3, 1, 2,
		1, 5, 2, 2, 5, 6,
		5, 4, 6, 6, 4, 7,
		4, 0, 7, 7, 0, 3,
		3, 2, 7, 7, 2, 6,
		4, 5, 0, 0, 5, 1
	};

	constexpr size_t vertexCount = 36;
	float vertices[3 * vertexCount];
	for (int i = 0; i < 36; i++) {
		vertices[i * 3 + 0] = edges[indices[i]].x;
		vertices[i * 3 + 1] = edges[indices[i]].y;
		vertices[i * 3 + 2] = edges[indices[i]].z;
	}

	float normals[3 * vertexCount];
	for (int i = 0; i < 36; i++) {
		normals[i * 3 + 0] = faceNormals[i / 6].x;
		normals[i * 3 + 1] = faceNormals[i / 6].y;
		normals[i * 3 + 2] = faceNormals[i / 6].z;
	}

	glm::vec4 colors[vertexCount];
	for (int iColor = 0; iColor < 6 * 6; ++iColor) {
		colors[iColor] = color;
	}

	Buffer3D buffer3D;

	CreateBuffer3DParams createCubeBufferParams;
	createCubeBufferParams.pVertices = reinterpret_cast<glm::vec3*>(vertices);
	createCubeBufferParams.pNormals = reinterpret_cast<glm::vec3*>(normals);
	createCubeBufferParams.pColors = colors;
	createCubeBufferParams.vertexCount = vertexCount;
	createBuffer3D(buffer3D, createCubeBufferParams);

	buffer(buffer3D, eDrawMode::Triangles, pModel);

	deleteBuffer3D(buffer3D);
}

void RenderApi3D::solidSphere(const glm::vec3& center, float radius, unsigned int horizontalSubdivisions, unsigned int verticalSubdivisions, const glm::vec4& color) const {
	
	horizontalSubdivisions = glm::max(horizontalSubdivisions, 4u);
	verticalSubdivisions = glm::max(verticalSubdivisions, 2u);

	const int vertexCount = 2 + horizontalSubdivisions * (verticalSubdivisions - 1);

	glm::vec3* vertices = (glm::vec3*)Allocator.Allocate(sizeof(glm::vec3) * vertexCount);
	glm::vec3* normals = (glm::vec3*)Allocator.Allocate(sizeof(glm::vec3) * vertexCount);
	glm::vec4* colors = (glm::vec4*)Allocator.Allocate(sizeof(glm::vec4) * vertexCount);

	int iVertex = 0;

	const float verticalStep = glm::pi<float>() / verticalSubdivisions;
	const float horizontalStep = glm::two_pi<float>() / horizontalSubdivisions;
	for (unsigned int i = 1; i < verticalSubdivisions; ++i) {
		const float verticalAngle = glm::half_pi<float>() - i * verticalStep;

		const float xz = radius * glm::cos(verticalAngle);
		const float y = radius * glm::sin(verticalAngle);

		for (unsigned int j = 0; j < horizontalSubdivisions; ++j) {
			const float horizontalAngle = j * horizontalStep;
			const float x = xz * glm::cos(horizontalAngle);
			const float z = xz * glm::sin(horizontalAngle);


			vertices[iVertex] = center + glm::vec3(x, y, z);
			normals[iVertex] = glm::vec3(x / radius, y / radius, z / radius);
			colors[iVertex] = color;
			iVertex++;
		}
	}

	vertices[iVertex] = center + glm::vec3(0.f, radius, 0.f);
	normals[iVertex] = glm::vec3(0.f, 1.f, 0.f);
	colors[iVertex] = color;
	iVertex++;

	vertices[iVertex] = center + glm::vec3(0.f, -radius, 0.f);
	normals[iVertex] = glm::vec3(0.f, -1.f, 0.f);
	colors[iVertex] = color;
	iVertex++;

	const unsigned int indexCount = (2 * horizontalSubdivisions + (verticalSubdivisions - 2) * horizontalSubdivisions * 2) * 3;
	unsigned int* indices = (unsigned int*)Allocator.Allocate(sizeof(unsigned int) * indexCount);

	unsigned int iIndex = 0;
	const int iFirstLine = 0;
	for (unsigned int j = 0; j < horizontalSubdivisions; ++j) {
		indices[iIndex++] = vertexCount - 2;
		indices[iIndex++] = iFirstLine + j;
		indices[iIndex++] = iFirstLine + ((j + 1) % horizontalSubdivisions);
	}

	for (unsigned int i = 0; i < verticalSubdivisions - 2; ++i) {
		for (unsigned int j = 0; j < horizontalSubdivisions; ++j) {
			const unsigned int iA = (i * horizontalSubdivisions) + j;
			const unsigned int iB = (i * horizontalSubdivisions) + ((j + 1) % horizontalSubdivisions);
			const unsigned int iC = ((i + 1) * horizontalSubdivisions) + j;
			const unsigned int iD = ((i + 1) * horizontalSubdivisions) + ((j + 1) % horizontalSubdivisions);
			indices[iIndex++] = iA;
			indices[iIndex++] = iC;
			indices[iIndex++] = iD;
			indices[iIndex++] = iA;
			indices[iIndex++] = iD;
			indices[iIndex++] = iB;
		}
	}

	const int iLastLine = (verticalSubdivisions - 2) * horizontalSubdivisions;
	for (unsigned int j = 0; j < horizontalSubdivisions; ++j) {
		indices[iIndex++] = vertexCount - 1;
		indices[iIndex++] = iLastLine + j;
		indices[iIndex++] = iLastLine + ((j + 1) % horizontalSubdivisions);
	}

	Buffer3D buffer3D;

	CreateBuffer3DParams createCubeBufferParams;
	createCubeBufferParams.pVertices = vertices;
	createCubeBufferParams.pNormals = normals;
	createCubeBufferParams.pColors = colors;
	createCubeBufferParams.pIndices = indices;
	createCubeBufferParams.vertexCount = vertexCount;
	createCubeBufferParams.indexCount = indexCount;
	createBuffer3D(buffer3D, createCubeBufferParams);

	buffer(buffer3D, eDrawMode::Triangles, nullptr);

	deleteBuffer3D(buffer3D);

	Allocator.Free(indices);
	Allocator.Free(colors);
	Allocator.Free(normals);
	Allocator.Free(vertices);
}

void RenderApi3D::bone(const glm::vec3& childRelativePosition, const glm::vec4& color, const glm::quat& parentAbsoluteRotation, const glm::vec3& parentAbsolutePosition) const {
	glm::vec3 newChildRelativePosition = childRelativePosition;

	const float size = glm::length(childRelativePosition) / 10.f;

	glm::vec3 front = glm::normalize(childRelativePosition);
	glm::vec3 left;
	glm::vec3 up;
	const float frontDot = glm::dot(front, glm::vec3(0.f, 1.f, 0.f));
	if (glm::abs(frontDot) != 1.f) {
		left = glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), front));
		up = glm::normalize(glm::cross(front, left));
	}
	else {
		up = glm::normalize(glm::cross(front, glm::vec3(1.f, 0.f, 0.f)));
		left = glm::normalize(glm::cross(up, front));
	}
	up *= size;
	front *= size;
	left *= size;

	up = parentAbsoluteRotation * glm::vec4(up, 0.f);
	left = parentAbsoluteRotation * glm::vec4(left, 0.f);
	front = parentAbsoluteRotation * glm::vec4(front, 0.f);
	newChildRelativePosition = parentAbsoluteRotation * glm::vec4(newChildRelativePosition, 0.f);

	glm::vec3 edges[] = {
		parentAbsolutePosition,
		parentAbsolutePosition + up + front,
		parentAbsolutePosition - up + front,
		parentAbsolutePosition + left + front,
		parentAbsolutePosition - left + front,
		parentAbsolutePosition + newChildRelativePosition,
	};

	unsigned int indices[] = {
		0, 1, 3,
		0, 4, 1,
		0, 3, 2,
		0, 2, 4,
		5, 3, 1,
		5, 1, 4,
		5, 2, 3,
		5, 4, 2,
	};
	const unsigned int vertexCount = COUNTOF(indices);

	glm::vec3 vertices[vertexCount];
	for (unsigned int i = 0; i < vertexCount; ++i) {
		vertices[i] = edges[indices[i]];
	}

	glm::vec3 normals[vertexCount];
	for (unsigned int i = 0; i < vertexCount; i += 3) {
		glm::vec3 normal = glm::normalize(glm::cross(edges[indices[i + 1]] - edges[indices[i + 0]], edges[indices[i + 2]] - edges[indices[i + 0]]));
		normals[i + 0] = normal;
		normals[i + 1] = normal;
		normals[i + 2] = normal;
	}

	glm::vec4 colors[vertexCount];
	for (unsigned int i = 0; i < vertexCount; ++i) {
		colors[i] = color;
	}

	Buffer3D buffer3D;

	CreateBuffer3DParams createCubeBufferParams;
	createCubeBufferParams.pVertices = vertices;
	createCubeBufferParams.pNormals = normals;
	createCubeBufferParams.pColors = colors;
	createCubeBufferParams.vertexCount = vertexCount;
	createBuffer3D(buffer3D, createCubeBufferParams);

	buffer(buffer3D, eDrawMode::Triangles, nullptr);

	deleteBuffer3D(buffer3D);
}

void RenderApi3D::horizontalPlane(const glm::vec3& center, const glm::vec2& size, unsigned int SideSubdivision, const glm::vec4& color) const {
	unsigned int NbVertexBySide = SideSubdivision + 1;
	unsigned int vertexCount = NbVertexBySide * NbVertexBySide;

	glm::vec3* vertices = (glm::vec3*)Allocator.Allocate(sizeof(glm::vec3) * vertexCount);
	glm::vec3* normals = (glm::vec3*)Allocator.Allocate(sizeof(glm::vec3) * vertexCount);
	glm::vec4* colors = (glm::vec4*)Allocator.Allocate(sizeof(glm::vec4) * vertexCount);

	unsigned int indiceCount = SideSubdivision * SideSubdivision * 6;
	unsigned int* indices = (unsigned int*)Allocator.Allocate(sizeof(unsigned int) * indiceCount);

	float fStepX = size.x / SideSubdivision;
	float fStepZ = size.y / SideSubdivision;
	glm::vec3& Start = glm::vec3(center.x - size.x * 0.5f, center.y, center.z - size.y * 0.5f);
	for (unsigned int iVertexX = 0; iVertexX < NbVertexBySide; ++iVertexX) {
		for (unsigned int iVertexZ = 0; iVertexZ < NbVertexBySide; ++iVertexZ) {
			unsigned int Indice = iVertexX * NbVertexBySide + iVertexZ;
			vertices[Indice] = { Start.x + iVertexX * fStepX, Start.y, Start.z + iVertexZ * fStepZ };
			normals[Indice] = { 0.f, 1.f, 0.f };
			colors[Indice] = color;
		}
	}

	for (unsigned int iSquareX = 0; iSquareX < SideSubdivision; ++iSquareX) {
		for (unsigned int iSquareY = 0; iSquareY < SideSubdivision; ++iSquareY) {
			unsigned int iVertexStart = iSquareX * NbVertexBySide + iSquareY;
			unsigned int iIndiceStart = (iSquareX * SideSubdivision + iSquareY) * 6;
			indices[iIndiceStart + 0] = iVertexStart;
			indices[iIndiceStart + 1] = iVertexStart + 1;
			indices[iIndiceStart + 2] = iVertexStart + NbVertexBySide + 1;
			indices[iIndiceStart + 3] = iVertexStart;
			indices[iIndiceStart + 4] = iVertexStart + NbVertexBySide + 1;
			indices[iIndiceStart + 5] = iVertexStart + NbVertexBySide;
		}
	}


	Buffer3D buffer3D;
	CreateBuffer3DParams createCubeBufferParams;
	createCubeBufferParams.pVertices = vertices;
	createCubeBufferParams.pNormals = normals;
	createCubeBufferParams.pColors = colors;
	createCubeBufferParams.pIndices = indices;
	createCubeBufferParams.vertexCount = vertexCount;
	createCubeBufferParams.indexCount = indiceCount;
	createBuffer3D(buffer3D, createCubeBufferParams);
	buffer(buffer3D, eDrawMode::Triangles, nullptr);

	deleteBuffer3D(buffer3D);
	Allocator.Free(indices);
	Allocator.Free(colors);
	Allocator.Free(normals);
	Allocator.Free(vertices);


}

void RenderApi2D::buffer(const Buffer2D& buffer, eDrawMode drawMode) const {
	assert(buffer.vao); // did you call createDrawBuffer2D ?
	glBindVertexArray(buffer.vao);
	glDrawArrays((GLenum)drawMode, 0, buffer.vertexCount);
	glBindVertexArray(0);
}

void RenderApi2D::lines(glm::vec2 const* vertices, unsigned int vertexCount, const glm::vec4& color) const {
	glm::vec4* colors = new glm::vec4[vertexCount];
	for (unsigned int i = 0; i < vertexCount; ++i) {
		colors[i] = color;
	}

	Buffer2D buffer2D;

	CreateBuffer2DParams createBufferParams;
	createBufferParams.pColors = colors;
	createBufferParams.pVertices = vertices;
	createBufferParams.vertexCount = vertexCount;
	createBuffer2D(buffer2D, createBufferParams);

	buffer(buffer2D, eDrawMode::Lines);

	deleteBuffer2D(buffer2D);

	delete[] colors;
}

void RenderApi2D::quadFill(const glm::vec2& min, const glm::vec2& max, const glm::vec4& color) const {
	glm::vec2 vertices[] = {
		{min.x, min.y},
		{max.x, min.y},
		{max.x, max.y},
		{min.x, min.y},
		{max.x, max.y},
		{min.x, max.y},
	};
	constexpr unsigned int vertexCount = COUNTOF(vertices);

	glm::vec4 colors[vertexCount];
	for (unsigned int iColor = 0; iColor < vertexCount; ++iColor) {
		colors[iColor] = color;
	}

	Buffer2D buffer2D;

	CreateBuffer2DParams createSquareBufferParams;
	createSquareBufferParams.pColors = colors;
	createSquareBufferParams.pVertices = vertices;
	createSquareBufferParams.vertexCount = vertexCount;
	createBuffer2D(buffer2D, createSquareBufferParams);

	buffer(buffer2D, eDrawMode::Triangles);

	deleteBuffer2D(buffer2D);
}

void RenderApi2D::quadContour(const glm::vec2& min, const glm::vec2& max, const glm::vec4& color) const {
	glm::vec2 vertices[] = {
		{min.x, min.y},
		{max.x, min.y},
		{max.x, min.y},
		{max.x, max.y},
		{max.x, max.y},
		{min.x, max.y},
		{min.x, max.y},
		{min.x, min.y},
	};
	constexpr unsigned int vertexCount = COUNTOF(vertices);

	lines(vertices, vertexCount, color);
}

void RenderApi2D::circleFill(const glm::vec2& center, float radius, unsigned int subdivisions, const glm::vec4& color) const {
	subdivisions = glm::max(subdivisions, 4u);

	const unsigned int vertexCount = subdivisions * 3;

	glm::vec2* vertices = new glm::vec2[vertexCount];
	glm::vec4* colors = new glm::vec4[vertexCount];

	int iVertex = 0;
	glm::vec2 prev = { center.x + radius, center.y };
	for (unsigned int i = 1; i <= subdivisions; ++i) {
		const glm::vec2 current = {
			center.x + radius * glm::cos(glm::two_pi<float>() * i / float(subdivisions)),
			center.y + radius * glm::sin(glm::two_pi<float>() * i / float(subdivisions)),
		};
		vertices[iVertex++] = center;
		vertices[iVertex++] = prev;
		vertices[iVertex++] = current;
		prev = current;
	}

	for (unsigned int iColor = 0; iColor < vertexCount; ++iColor) {
		colors[iColor] = color;
	}

	Buffer2D buffer2D;

	CreateBuffer2DParams createSquareBufferParams;
	createSquareBufferParams.pColors = colors;
	createSquareBufferParams.pVertices = vertices;
	createSquareBufferParams.vertexCount = vertexCount;
	createBuffer2D(buffer2D, createSquareBufferParams);

	buffer(buffer2D, eDrawMode::Triangles);

	deleteBuffer2D(buffer2D);

	delete[] vertices;
	delete[] colors;
}

void RenderApi2D::circleContour(const glm::vec2& center, float radius, unsigned int subdivisions, const glm::vec4& color) const {
	subdivisions = glm::max(subdivisions, 4u);

	const unsigned int vertexCount = subdivisions * 2;

	glm::vec2* vertices = new glm::vec2[vertexCount];
	
	int iVertex = 0;
	glm::vec2 prev = { center.x + radius, center.y };
	for (unsigned int i = 1; i <= subdivisions; ++i) {
		const glm::vec2 current = {
			center.x + radius * glm::cos(glm::two_pi<float>() * i / float(subdivisions)),
			center.y + radius * glm::sin(glm::two_pi<float>() * i / float(subdivisions)),
		};
		vertices[iVertex++] = prev;
		vertices[iVertex++] = current;
		prev = current;
	}

	lines(vertices, vertexCount, color);

	delete[] vertices;
}

void RenderApi2D::arrow(const glm::vec2& from, const glm::vec2& to, float thickness, float hatRatio /*between 0 and 1*/, const glm::vec4& color) const {

	glm::vec2 dir = to - from;
	const float length = glm::length(dir);
	const float hatSize = hatRatio * length;
	dir /= length;
	const glm::vec2 ortho = { -dir.y, dir.x };
	dir *= (length - hatSize);


	glm::vec2 vertices[] = {
		from - 0.5f * thickness * ortho,
		from + 0.5f * thickness * ortho,
		from + dir + 0.5f * thickness * ortho,
		from + dir + 0.5f * thickness * ortho,
		from + dir - 0.5f * thickness * ortho,
		from - 0.5f * thickness * ortho,
		from + dir - thickness * ortho,
		from + dir + thickness * ortho,
		to,
	};

	constexpr unsigned int vertexCount = COUNTOF(vertices);

	glm::vec4 colors[vertexCount];
	for (unsigned int iColor = 0; iColor < vertexCount; ++iColor) {
		colors[iColor] = color;
	}

	Buffer2D buffer2D;

	CreateBuffer2DParams createSquareBufferParams;
	createSquareBufferParams.pColors = colors;
	createSquareBufferParams.pVertices = vertices;
	createSquareBufferParams.vertexCount = vertexCount;
	createBuffer2D(buffer2D, createSquareBufferParams);

	buffer(buffer2D, eDrawMode::Triangles);

	deleteBuffer2D(buffer2D);
}