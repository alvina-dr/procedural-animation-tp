#include "shader.h"
#include "glad.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#ifndef SHADER_PATH
#define SHADER_PATH
#endif

namespace {
	// No windows implementation of strsep
	char* strsep_custom(char** stringp, const char* delim) {
		char* s;
		const char* spanp;
		int c, sc;
		char* tok;
		if ((s = *stringp) == nullptr)
			return nullptr;
		for (tok = s; ; ) {
			c = *s++;
			spanp = delim;
			do {
				if ((sc = *spanp++) == c) {
					if (c == 0)
						s = nullptr;
					else
						s[-1] = 0;
					*stringp = s;
					return (tok);
				}
			} while (sc != 0);
		}
		return nullptr;
	}

	int checkCompileError(GLuint shader, const char** sourceBuffer) {
		// Get error log size and print it eventually
		int logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 1)
		{
			char* log = new char[logLength];
			glGetShaderInfoLog(shader, logLength, &logLength, log);
			char* token, * string;
			string = _strdup(sourceBuffer[0]);
			int lc = 0;
			while ((token = strsep_custom(&string, "\n")) != NULL) {
				printf("%3d : %s\n", lc, token);
				++lc;
			}
			fprintf(stderr, "Compile : %s", log);
			delete[] log;
		}
		// If an error happend quit
		int status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
			return -1;
		return 0;
	}

	GLuint compileShader(GLenum shaderType, const char* sourceBuffer, int bufferSize) {
		GLuint shaderObject = glCreateShader(shaderType);
		const char* sc[1] = { sourceBuffer };
		glShaderSource(shaderObject, 1, sc, NULL);
		glCompileShader(shaderObject);
		checkCompileError(shaderObject, sc);
		return shaderObject;
	}

	GLuint compileShaderFromFile(GLenum shaderType, const char* path) {
		FILE* shaderFileDesc = fopen(path, "rb");
		if (!shaderFileDesc) {
			fprintf(stderr, "Failed to open file %s \n", path);
			return 0;
		}

		fseek(shaderFileDesc, 0, SEEK_END);
		long fileSize = ftell(shaderFileDesc);
		rewind(shaderFileDesc);
		char* buffer = new char[fileSize + 1];
		fread(buffer, 1, fileSize, shaderFileDesc);
		buffer[fileSize] = '\0';
		GLuint shaderObject = compileShader(shaderType, buffer, fileSize);
		delete[] buffer;
		fclose(shaderFileDesc);
		return shaderObject;
	}

	bool checkLinkError(GLuint program) {
		// Get link error log size and print it eventually
		int logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 1) {
			char* log = new char[logLength];
			glGetProgramInfoLog(program, logLength, &logLength, log);
			fprintf(stderr, "Link : %s \n", log);
			delete[] log;
		}
		int status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
			return false;
		return true;
	}
}

bool createShaderProgram(ShaderProgram& program, const CreateShaderProgramParams& params) {
	// try to load and compile shaders
	program.vertShaderId = compileShaderFromFile(GL_VERTEX_SHADER, params.szVertFilePath);
	program.fragShaderId = compileShaderFromFile(GL_FRAGMENT_SHADER, params.szFragFilePath);
	program.programId = glCreateProgram();
	glAttachShader(program.programId, program.vertShaderId);
	glAttachShader(program.programId, program.fragShaderId);
	glLinkProgram(program.programId);
	if (!checkLinkError(program.programId)) {
		return false;
	}

	return true;
}

void	 ShaderProgram3D::LoadLocation() {
	modelLocation = glGetUniformLocation(programId, "Model");
	viewLocation = glGetUniformLocation(programId, "View");
	projectionLocation = glGetUniformLocation(programId, "Projection");
	lightDirLocation = glGetUniformLocation(programId, "LightDir");
	lightStrengthLocation = glGetUniformLocation(programId, "LightStrength");
	ambientLocation = glGetUniformLocation(programId, "Ambient");
	specularLocation = glGetUniformLocation(programId, "Specular");
	specularPowLocation = glGetUniformLocation(programId, "SpecularPow");
	lightingEnabledLocation = glGetUniformLocation(programId, "LightingEnabled");
}

bool createShaderProgram3D(ShaderProgram3D& program) {
	CreateShaderProgramParams params;
	params.szVertFilePath = SHADER_PATH "shader_3d.vert";
	params.szFragFilePath = SHADER_PATH "shader_3d.frag";
	if (!createShaderProgram(program, params)) {
		assert(false);
		return false;
	}

	// Upload uniforms
	program.LoadLocation();
	return true;
}

void	 ShaderProgram3D_custom::LoadLocation() {
	ShaderProgram3D::LoadLocation();
	timeLocation = glGetUniformLocation(programId, "Time");
}

bool createShaderProgram3D_custom(ShaderProgram3D_custom& program) {
	CreateShaderProgramParams params;
	params.szVertFilePath = SHADER_PATH "shader_3d_custom.vert";
	params.szFragFilePath = SHADER_PATH "shader_3d.frag";
	if (!createShaderProgram(program, params)) {
		assert(false);
		return false;
	}
	// Upload uniforms
	program.LoadLocation();
	return true;
}

bool createShaderProgram2D(ShaderProgram2D& program) {
	CreateShaderProgramParams params;
	params.szVertFilePath = SHADER_PATH "shader_2d.vert";
	params.szFragFilePath = SHADER_PATH "shader_2d.frag";
	if (!createShaderProgram(program, params)) {
		assert(false);
		return false;
	}

	program.viewportSizeLocation = glGetUniformLocation(program.programId, "ViewportSize");

	return true;
}
