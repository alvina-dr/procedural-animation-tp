#include "../viewer.h"
#include "../drawbuffer.h"
#include "../renderapi.h"

#include <random>
#include <time.h>
#include <vector>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

#define COUNTOF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

constexpr char const* clothViewerName = "ClothViewer";

struct ClothVertexShaderAdditionalData
{
	glm::vec3 Pos;
	/// beware of alignement (std430 rule)
};

struct ClothViewer : Viewer
{
	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;

	ClothVertexShaderAdditionalData additionalShaderData;

	ClothViewer() : Viewer(clothViewerName, 1280, 720) {}

	void init() override {
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;

		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };
	}

	void update(double elapsedTime) override
	{
		leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		altKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		mousePos = { float(mouseX), viewportHeight - float(mouseY) };

		pCustomShaderData = &additionalShaderData;
		CustomShaderDataSize = sizeof(BoidsVertexShaderAdditionalData);

	}

	void render3D_custom(const RenderApi3D& api) const override
	{
		//Here goes your drawcalls affected by the custom vertex shader
	}

	void render3D(const RenderApi3D& api) const override {
		api.horizontalPlane({ 0, 0, 0 }, { 10, 10 }, 1, glm::vec4(0.9f, 0.9f, 0.9f, 1.f));
		api.grid(10.f, 10, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);
		api.axisXYZ(nullptr);

	}

	void render2D(const RenderApi2D& api) const override {

		constexpr float padding = 50.f;

		if (altKeyPressed) {
			if (leftMouseButtonPressed) {
				api.circleFill(mousePos, padding, 10, boidsWhite);
			}
			else {
				api.circleContour(mousePos, padding, 10, boidsWhite);
			}

		}
		else {
			const glm::vec2 min = mousePos + glm::vec2(padding, padding);
			const glm::vec2 max = mousePos + glm::vec2(-padding, -padding);
			if (leftMouseButtonPressed) {
				api.quadFill(min, max, boidsWhite);
			}
			else {
				api.quadContour(min, max, boidsWhite);
			}
		}

		{
			const glm::vec2 from = { viewportWidth * 0.5f, padding };
			const glm::vec2 to = { viewportWidth * 0.5f, 2.f * padding };
			constexpr float thickness = padding * 0.25f;
			constexpr float hatRatio = 0.3f;
			api.arrow(from, to, thickness, hatRatio, boidsWhite);
		}

		{
			glm::vec2 vertices[] = {
				{ padding, viewportHeight - padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth - padding, viewportHeight - padding },
			};
			api.lines(vertices, COUNTOF(vertices), boidsWhite);
		}
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("3D Sandbox");

		ImGui::Checkbox("Show demo window", &showDemoWindow);
		ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);

		ImGui::Separator();

		float fovDegrees = glm::degrees(camera.fov);
		if (ImGui::SliderFloat("Camera field of fiew (degrees)", &fovDegrees, 15, 180)) {
			camera.fov = glm::radians(fovDegrees);
		}


		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};