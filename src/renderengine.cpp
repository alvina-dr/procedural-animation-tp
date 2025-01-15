#include "renderengine.h"
#include "drawbuffer.h"
#include "camera.h"
#include "renderapi.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


bool createRenderEngine(RenderEngine& engine) {
	if (!createShaderProgram3D(engine.shader3D)) {
		return false;
	}
	if (!createShaderProgram3D_custom(engine.shader3D_custom)) {
		return false;
	}
	if (!createShaderProgram2D(engine.shader2D)) {
		return false;
	}
	return true;
}

bool reloadRenderEngineShaders(RenderEngine& engine) {
	glDeleteProgram(engine.shader3D.programId);
	glDeleteProgram(engine.shader3D_custom.programId);
	glDeleteProgram(engine.shader2D.programId);
	return createRenderEngine(engine);
}

void renderEngineFrame(const RenderEngine& engine, const RenderParams& params) {
	if(!params.viewportWidth || !params.viewportHeight) {
		return;
	}
	glViewport(0, 0, params.viewportWidth, params.viewportHeight);

	// Clear the front buffer
	glClearColor(params.backgroundColor.r, params.backgroundColor.g, params.backgroundColor.b, params.backgroundColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPointSize(params.pointSize);
	glLineWidth(params.lineWidth);

	// save previous gl state
	GLboolean bEnableBlend;
	glGetBooleanv(GL_BLEND, &bEnableBlend);
	GLint SrcFactorRGB;
	glGetIntegerv(GL_BLEND_SRC_RGB, &SrcFactorRGB);
	GLint DstFactorRGB;
	glGetIntegerv(GL_BLEND_DST_RGB, &DstFactorRGB);
	GLint SrcFactorAlpha;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &SrcFactorAlpha);
	GLint DstFactorAlpha;
	glGetIntegerv(GL_BLEND_DST_ALPHA, &DstFactorAlpha);

	GLboolean bDepthTest;
	glGetBooleanv(GL_DEPTH_TEST, &bDepthTest);

	// set gl state
	glEnable(GL_BLEND);

	glBlendFuncSeparate(
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,   // src/dst rgb
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA    // src/dst alpha
	);

	// 3d
	{
		glEnable(GL_DEPTH_TEST);

		const Camera& camera = *params.pCamera;
		glm::mat4 projection = glm::perspective(camera.fov, params.viewportWidth / float(params.viewportHeight), 0.1f, 100.f);
		glm::mat4 view = glm::lookAt(camera.eye, camera.o, camera.up);
		glm::mat4 viewRot = glm::lookAt(glm::vec3(0,0,0), camera.o - camera.eye, camera.up);
		glm::vec4 lightDirViewSpace = (viewRot * params.lightDirection);

		const ShaderProgram3D& shader3D = engine.shader3D;

		glUseProgram(shader3D.programId);

		glProgramUniformMatrix4fv(shader3D.programId, shader3D.viewLocation, 1, 0, glm::value_ptr(view));
		glProgramUniformMatrix4fv(shader3D.programId, shader3D.projectionLocation, 1, 0, glm::value_ptr(projection));

		glm::vec3 lightViewSpaceVec3 = glm::vec3(lightDirViewSpace);
		glProgramUniform3fv(shader3D.programId, shader3D.lightDirLocation, 1, glm::value_ptr(lightViewSpaceVec3));
		glProgramUniform1f(shader3D.programId, shader3D.lightStrengthLocation, params.lightStrength);
		glProgramUniform1f(shader3D.programId, shader3D.ambientLocation, params.lightAmbient);
		glProgramUniform1f(shader3D.programId, shader3D.specularLocation, params.specular);
		glProgramUniform1f(shader3D.programId, shader3D.specularPowLocation, params.specularPow);

		RenderApi3D api3D;
		api3D.pShader3D = &shader3D;
		api3D.pRenderEngine = &engine;
		params.render3DCallback(api3D, params.pRender3DCallbackUserData);

		// 3D Custom vertex shader
		const ShaderProgram3D_custom& shader3D_custom = engine.shader3D_custom;
		glUseProgram(shader3D_custom.programId);
		glProgramUniformMatrix4fv(shader3D_custom.programId, shader3D_custom.viewLocation, 1, 0, glm::value_ptr(view));
		glProgramUniformMatrix4fv(shader3D_custom.programId, shader3D_custom.projectionLocation, 1, 0, glm::value_ptr(projection));
	
		glProgramUniform3fv(shader3D_custom.programId, shader3D_custom.lightDirLocation, 1, glm::value_ptr(lightViewSpaceVec3));
		glProgramUniform1f(shader3D_custom.programId, shader3D_custom.lightStrengthLocation, params.lightStrength);
		glProgramUniform1f(shader3D_custom.programId, shader3D_custom.ambientLocation, params.lightAmbient);
		glProgramUniform1f(shader3D_custom.programId, shader3D_custom.specularLocation, params.specular);
		glProgramUniform1f(shader3D_custom.programId, shader3D_custom.specularPowLocation, params.specularPow);
		glProgramUniform1f(shader3D_custom.programId, shader3D_custom.timeLocation, params.time);
		
		GLuint ssbo = 0;
		glGenBuffers(1, &ssbo);
		if (params.pCustomVertShaderData != nullptr) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, params.CustomVertShaderDataSize, params.pCustomVertShaderData, GL_DYNAMIC_COPY);
		}
		api3D.pShader3D = &shader3D_custom;
		params.render3DCustomCallback(api3D, params.pRender3DCustomCallbackUserData);
		glDeleteBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// 2d
	{
		glDisable(GL_DEPTH_TEST);

		const ShaderProgram2D& shader2D = engine.shader2D;

		glUseProgram(shader2D.programId);

		glm::vec2 viewportSize = {
			float(params.viewportWidth),
			float(params.viewportHeight),
		};
		glProgramUniform2fv(shader2D.programId, shader2D.viewportSizeLocation, 1, glm::value_ptr(viewportSize));

		RenderApi2D api2D;
		api2D.pRenderEngine = &engine;
		params.render2DCallback(api2D, params.pRender3DCallbackUserData);
	}

	// restore gl state
	if (bEnableBlend) {
		glEnable(GL_BLEND);
	}
	else {
		glDisable(GL_BLEND);
	}

	if (bDepthTest) {
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}
}