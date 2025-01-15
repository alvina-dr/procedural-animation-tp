#pragma once

#include <glad.h>

struct ShaderProgram {
	GLuint vertShaderId;
	GLuint fragShaderId;
	GLuint programId;
};

struct CreateShaderProgramParams {
	char const* szVertFilePath;
	char const* szFragFilePath;
};

bool createShaderProgram(ShaderProgram& program, const CreateShaderProgramParams& params);

struct ShaderProgram3D : ShaderProgram {
	GLuint modelLocation;
	GLuint viewLocation;
	GLuint projectionLocation;
	GLuint lightDirLocation;
	GLuint lightStrengthLocation;
	GLuint ambientLocation;
	GLuint specularLocation;
	GLuint specularPowLocation;
	GLuint lightingEnabledLocation;

	void	 LoadLocation();
};

bool createShaderProgram3D(ShaderProgram3D& program);

struct ShaderProgram3D_custom : ShaderProgram3D {
	GLuint timeLocation;
	void	 LoadLocation();
};

bool createShaderProgram3D_custom(ShaderProgram3D_custom& program);

struct ShaderProgram2D : ShaderProgram {
	GLuint viewportSizeLocation;
};

bool createShaderProgram2D(ShaderProgram2D& program);

