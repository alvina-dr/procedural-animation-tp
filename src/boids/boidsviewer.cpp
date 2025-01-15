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

constexpr char const* boidsViewerName = "BoidsViewer";
constexpr glm::vec4 boidsWhite = { 1.f, 1.f, 1.f, 1.f };
constexpr glm::vec4 boidsBlue = { 0.f, 0.f, 1.f, 1.f };
constexpr glm::vec4 boidsGreen = { 0.f, 1.f, 0.f, 1.f };
constexpr glm::vec4 boidsRed = { 1.f, 0.f, 0.f, 1.f };

const int numBoids = 100;

class Boid {
public:
	glm::vec3 Position;
	glm::vec3 Velocity;
	Boid(glm::vec3 position, glm::vec3 velocity) : Position(position), Velocity(velocity) { }
};


struct BoidsVertexShaderAdditionalData {
	glm::vec3 Pos;
	/// beware of alignement (std430 rule)
};

struct BoidsViewer : Viewer {

	glm::vec3 jointPosition;
	glm::vec3 cubePosition;
	float boneAngle;

	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;

	// tweakable data
	float visualRange = 4;
	float speedLimit = .3;
	float minDistance = 1; // The distance to stay away from other boids
	float avoidFactor = 0.003; // Adjust velocity by this %
	float turnFactor = .5f; // To stay in the bounds
	float centeringFactor = 0.03; // (fly towards center) Adjust velocity by this %

	std::vector<Boid*> boidList = { };

	BoidsVertexShaderAdditionalData additionalShaderData;

	BoidsViewer() : Viewer(boidsViewerName, 1280, 720) {}

	void init() override {
		cubePosition = glm::vec3(1.f, 0.25f, -1.f);
		jointPosition = glm::vec3(-1.f, 2.f, -1.f);
		boneAngle = 0.f;
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;

		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };
		for (size_t i = 0; i < numBoids; i++)
		{
			boidList.push_back(new Boid(glm::vec3(std::rand() % 20 - 10, std::rand() % 20 - 10, std::rand() % 20 - 10),
				glm::vec3((std::rand() % 2 - 1) * .1f, (std::rand() % 2 - 1) * .1f, (std::rand() % 2 - 1) * .1f)));
		}
	}

	float distance(Boid* boid1, Boid* boid2) {
		float xSqr = (boid1->Position.x - boid2->Position.x) * (boid1->Position.x - boid2->Position.x);
		float ySqr = (boid1->Position.y - boid2->Position.y) * (boid1->Position.y - boid2->Position.y);
		float zSqr = (boid1->Position.z - boid2->Position.z) * (boid1->Position.z - boid2->Position.z);

		float sqr = xSqr + ySqr + zSqr;

		return sqrt(sqr);
	}

	void flyTowardsCenter(Boid* boid) {

		glm::vec3 center = glm::vec3(0, 0, 0);
		int numNeighbors = 0;

		for (size_t i = 0; i < boidList.size(); i++) 
		{
			if (distance(boid, boidList[i]) < visualRange) {
				center += boidList[i]->Position;
				numNeighbors += 1;
			}
		}

		if (numNeighbors) {
			center = glm::vec3(center.x / numNeighbors, center.y / numNeighbors, center.z / numNeighbors);

			boid->Velocity.x += (center.x - boid->Position.x) * centeringFactor;
			boid->Velocity.y += (center.y - boid->Position.y) * centeringFactor;
			boid->Velocity.z += (center.z - boid->Position.z) * centeringFactor;
		}
	}

	// Speed will naturally vary in flocking behavior, but real animals can't go
	// arbitrarily fast.
	void limitSpeed(Boid* boid) {

		float speed = sqrt(boid->Velocity.x * boid->Velocity.x + boid->Velocity.y * boid->Velocity.y + boid->Velocity.z * boid->Velocity.z);
		if (speed > speedLimit) {
			boid->Velocity.x = (boid->Velocity.x / speed) * speedLimit;
			boid->Velocity.y = (boid->Velocity.y / speed) * speedLimit;
			boid->Velocity.z = (boid->Velocity.z / speed) * speedLimit;
		}
		//std::cout << "velocity : " << boid->Velocity.x << ", " << boid->Velocity.y << ", " << boid->Velocity.z << std::endl;
	}

	// Move away from other boids that are too close to avoid colliding
	void avoidOthers(Boid* boid) {

		float moveX = 0;
		float moveY = 0;
		float moveZ = 0;

		for (size_t i = 0; i < boidList.size(); i++) 
		{
			if (boidList[i] != boid) {
				if (distance(boid, boidList[i]) < minDistance) {
					moveX += boid->Position.x - boidList[i]->Position.x;
					moveY += boid->Position.y - boidList[i]->Position.y;
					moveZ += boid->Position.z - boidList[i]->Position.z;
				}
			}
		}

		boid->Velocity.x += moveX * avoidFactor;
		boid->Velocity.y += moveY * avoidFactor;
		boid->Velocity.z += moveZ * avoidFactor;
	}

	// Constrain a boid to within the window. If it gets too close to an edge,
	// nudge it back in and reverse its direction.
	void keepWithinBounds(Boid* boid) {
		glm::vec3 bounds = glm::vec3(10, 10, 10);

		if (boid->Position.x < -bounds.x / 2) {
			boid->Velocity.x += turnFactor;
		}
		else if (boid->Position.x > bounds.x / 2) {
			boid->Velocity.x -= turnFactor;
		}

		if (boid->Position.y < 0) {
			boid->Velocity.y += turnFactor;
		}
		else if (boid->Position.y > bounds.y) {
			boid->Velocity.y -= turnFactor;
		}

		if (boid->Position.z < -bounds.z / 2) {
			boid->Velocity.z += turnFactor;
		}
		else if (boid->Position.z > bounds.z / 2) {
			boid->Velocity.z -= turnFactor;
		}
	}

	void update(double elapsedTime) override {
		boneAngle = (float)elapsedTime;

		leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		altKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		mousePos = { float(mouseX), viewportHeight - float(mouseY) };

		pCustomShaderData = &additionalShaderData;
		CustomShaderDataSize = sizeof(BoidsVertexShaderAdditionalData);

		for (size_t i = 0; i < boidList.size(); i++)
		{
			flyTowardsCenter(boidList[i]);
			avoidOthers(boidList[i]);
			limitSpeed(boidList[i]);
			keepWithinBounds(boidList[i]);
		}

		for (size_t i = 0; i < boidList.size(); i++)
		{
			boidList[i]->Position += boidList[i]->Velocity;
		}
	}

	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, glm::vec4(0.0f, 0.2f, 1.f, 1.f));
	}

	void render3D(const RenderApi3D& api) const override {
		api.horizontalPlane({ 0, 0, 0 }, { 10, 10 }, 1, glm::vec4(0.9f, 0.9f, 0.9f, 1.f));

		api.grid(10.f, 10, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);

		api.axisXYZ(nullptr);

		constexpr float cubeSize = 10;
		glm::mat4 cubeModelMatrix = glm::translate(glm::identity<glm::mat4>(), cubePosition);

		for (size_t i = 0; i < boidList.size(); i++)
		{
			api.solidSphere(boidList[i]->Position, 0.2f, 10, 10, boidsGreen);
			glm::vec3 vertices[2] =
			{
				boidList[i]->Position,
				boidList[i]->Position + boidList[i]->Velocity * glm::vec3(2, 2, 2)
			};
			api.lines(vertices, 24, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);
		}

		glm::vec3 vertices[24] =
		{
			//Bottom sqare
			glm::vec3(-cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, 0 ,cubeSize / 2),

			glm::vec3(-cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(cubeSize / 2, 0 ,cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(cubeSize / 2, 0,-cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, 0 ,-cubeSize / 2),

			//Bottom to top
			glm::vec3(-cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,-cubeSize / 2),

			glm::vec3(-cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize ,-cubeSize / 2),

			//Top Square

			glm::vec3(-cubeSize / 2, cubeSize ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(-cubeSize / 2, cubeSize ,cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(cubeSize / 2, cubeSize ,cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize,-cubeSize / 2),

			glm::vec3(cubeSize / 2, cubeSize ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,-cubeSize / 2),
		};
		api.lines(vertices, 24, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);
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


		ImGui::SliderFloat("Visual Range", &visualRange, 0.f, 10.f);
		ImGui::SliderFloat("Speed Limit", &speedLimit, 0.f, 2.0f);
		ImGui::SliderFloat("Min distance", &minDistance, 0.f, 10.0f);
		ImGui::SliderFloat("Avoid Factor", &avoidFactor, 0.f, 1.0f);
		ImGui::SliderFloat("Turn Factor", &turnFactor, 0.f, 1.0f);
		ImGui::SliderFloat("Centering Factor", &centeringFactor, 0.000f, 0.01f);
		ImGui::Separator();

		ImGui::Separator();
		ImGui::SliderFloat3("CustomShader_Pos", &additionalShaderData.Pos.x, -10.f, 10.f);
		ImGui::Separator();
		float fovDegrees = glm::degrees(camera.fov);
		if (ImGui::SliderFloat("Camera field of fiew (degrees)", &fovDegrees, 15, 180)) {
			camera.fov = glm::radians(fovDegrees);
		}

		ImGui::SliderFloat3("Cube Position", (float(&)[3])cubePosition, -1.f, 1.f);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};