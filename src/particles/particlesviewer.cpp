#include "../viewer.h"
#include "../drawbuffer.h"
#include "../renderapi.h"

#include <time.h>
#include <iostream>
#include <algorithm>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>

#define COUNTOF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

constexpr char const* particlesViewerName = "ParticlesViewer";
constexpr glm::vec4 particlesWhite = { 1.f, 1.f, 1.f, 1.f };
constexpr glm::vec4 particlesBlue = { 0.f, 0.f, 1.f, 1.f };
constexpr glm::vec4 particlesGreen = { 0.f, 1.f, 0.f, 1.f };
constexpr glm::vec4 particlesRed = { 1.f, 0.f, 0.f, 1.f };

struct ParticlesVertexShaderAdditionalData {
	glm::vec3 Pos;
	/// beware of alignement (std430 rule)
};

struct VoidPoint {
	VoidPoint(float x, float y, float z, float strength) : Position(x, y, z), Strength(strength) {}
	glm::vec3 Position;
	float Strength;
};

class Particle {

public:
	bool isSimulated = false;
	glm::vec3 Position;
	glm::vec3 Velocity;
	const float delta= 0.01f;
	Particle(float x, float y, float z) {
		Position = glm::vec3(x,y,z);
		Velocity = glm::vec3(0, 0, 0);
	}
	void Update() {
		if (!isSimulated) {
			return;
		}
		//Velocity += glm::vec3(0,-9, 0);
		Velocity *= 0.999f;
		Position += Velocity * delta;
	}
	void AttractTo(VoidPoint voidPoint) {
		float dist = distance(voidPoint.Position, Position);
		Velocity +=  normalize(voidPoint.Position - Position) * std::clamp(1/dist, 0.f,5.f) * voidPoint.Strength;
	}

	void AddVelocity(glm::vec3 velocity) {
		Velocity += velocity;
	}
};

struct ParticlesViewer : Viewer {

	std::vector<Particle*> particles;
	std::vector<VoidPoint> voidPoints;
	VoidPoint* voidPointCreating;

	float particleSize = 0.1f;
	int BoundsSize = 5.f;
	int ParticleVeloRandom = 5.f;
	int VoidStrgRandom = 5.f;

	//Inputs
	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;

	ParticlesVertexShaderAdditionalData additionalShaderData;

	ParticlesViewer() : Viewer(particlesViewerName, 1280, 720) {}

	void init() override {
		//Inputs
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;

		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };

		//particles
		particles = {
			new Particle(1,5,1),
		};

		voidPoints = {
			VoidPoint(0,0,0,1),
		};
	}

	void update(double elapsedTime) override {

		leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		altKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		mousePos = { float(mouseX), viewportHeight - float(mouseY) };
		glm::vec2 InGameMousePos = glm::vec2((mousePos.x-viewportWidth/2) / viewportWidth * 5, 5+(mousePos.y-viewportHeight/2) / viewportHeight*5);


		pCustomShaderData = &additionalShaderData;
		CustomShaderDataSize = sizeof(ParticlesVertexShaderAdditionalData);


		//create particle from mouse pos


		for (Particle* particle: particles) {
			particle->Update();
			for (auto element: voidPoints) {
				particle->AttractTo(element);
			}
		}
	}

	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, glm::vec4(0.0f, 0.2f, 1.f, 1.f));
	}

	void render3D(const RenderApi3D& api) const override {
		/*api.horizontalPlane({ 0, 0, 0 }, { 10, 10 }, 1, glm::vec4(0.9f, 0.9f, 0.9f, 1.f));

		api.grid(10.f, 10, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);*/

		api.axisXYZ(nullptr);

		for (Particle* particle: particles) {
			api.solidSphere(particle->Position, particleSize, 100, 100, particlesRed);
		}
		for (VoidPoint voidPoint: voidPoints) {
			api.solidSphere(voidPoint.Position, particleSize, 100, 100, particlesWhite);
		}

	}

	void render2D(const RenderApi2D& api) const override {

		constexpr float padding = 50.f;

		if (altKeyPressed) {
			if (leftMouseButtonPressed) {
				api.circleFill(mousePos, padding, 10, particlesWhite);
			}
			else {
				api.circleContour(mousePos, padding, 10, particlesWhite);
			}

		}
		else {
			const glm::vec2 min = mousePos + glm::vec2(padding, padding);
			const glm::vec2 max = mousePos + glm::vec2(-padding, -padding);
			if (leftMouseButtonPressed) {
				api.quadFill(min, max, particlesWhite);
			}
			else {
				api.quadContour(min, max, particlesWhite);
			}
		}

		{
			const glm::vec2 from = { viewportWidth * 0.5f, padding };
			const glm::vec2 to = { viewportWidth * 0.5f, 2.f * padding };
			constexpr float thickness = padding * 0.25f;
			constexpr float hatRatio = 0.3f;
			api.arrow(from, to, thickness, hatRatio, particlesWhite);
		}

		{
			glm::vec2 vertices[] = {
				{ padding, viewportHeight - padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth - padding, viewportHeight - padding },
			};
			api.lines(vertices, COUNTOF(vertices), particlesWhite);
		}
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("3D Sandbox");

		if (ImGui::Button("Erase last particle") && particles.size() > 0) {
			particles.pop_back();
		}
		if (ImGui::Button("Erase last void") && voidPoints.size() > 0) {
			voidPoints.pop_back();
		}
		if (ImGui::Button("Create random particle")) {
			glm::vec3 randomPos = glm::vec3((float) rand()/RAND_MAX-0.5f, (float) rand()/RAND_MAX-0.5f,(float) rand()/RAND_MAX-0.5f);
			Particle* particle = new Particle(randomPos.x*BoundsSize,randomPos.y*BoundsSize,randomPos.z*BoundsSize);
			randomPos = glm::vec3((float) rand()/RAND_MAX-0.5f, (float) rand()/RAND_MAX-0.5f,(float) rand()/RAND_MAX-0.5f);
			particle->AddVelocity(glm::vec3(randomPos.x*ParticleVeloRandom,randomPos.y*ParticleVeloRandom,randomPos.z*ParticleVeloRandom));
			particle->isSimulated = true;
			particles.push_back(particle);
		}
		if (ImGui::Button("Create random void point")) {
			glm::vec3 randomPos = glm::vec3((float) rand()/RAND_MAX-0.5f, (float) rand()/RAND_MAX-0.5f,(float) rand()/RAND_MAX-0.5f);
			VoidPoint voidPoint = VoidPoint(randomPos.x*BoundsSize,randomPos.y*BoundsSize,randomPos.z*BoundsSize, (float) rand()/RAND_MAX * VoidStrgRandom);

			voidPoints.push_back(voidPoint);
		}

		ImGui::SliderInt("Bounds Size", &BoundsSize, 0.f, 10.f);
		ImGui::SliderInt("Start Velocity", &ParticleVeloRandom, 0.f, 50.f);
		ImGui::SliderInt("Void Point Random", &VoidStrgRandom, 0.f, 10.f);

		if (ImGui::CollapsingHeader("3D Sandbox param")) {
			ImGui::Checkbox("Show demo window", &showDemoWindow);

			ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);


			ImGui::SliderFloat("Point size", &pointSize, 0.1f, 10.f);
			ImGui::SliderFloat("Line Width", &lineWidth, 0.1f, 10.f);
			ImGui::Separator();
		}
		if (ImGui::CollapsingHeader("Light")) {
			ImGui::SliderFloat3("Light dir", (float(&)[3])lightDir, -1.f, 1.f);
			ImGui::SliderFloat("Light Strength", &lightStrength, 0.f, 2.f);
			ImGui::SliderFloat("Ligh Ambient", &lightAmbient, 0.f, 0.5f);
			ImGui::SliderFloat("Ligh Specular", &specular, 0.f, 1.f);
			ImGui::SliderFloat("Ligh Specular Pow", &specularPow, 1.f, 200.f);
			ImGui::Separator();
			ImGui::SliderFloat3("CustomShader_Pos", &additionalShaderData.Pos.x, -10.f, 10.f);
			ImGui::Separator();
		}

		float fovDegrees = glm::degrees(camera.fov);
		if (ImGui::SliderFloat("Camera field of fiew (degrees)", &fovDegrees, 15, 180)) {
			camera.fov = glm::radians(fovDegrees);
		}

		//ImGui::SliderFloat3("Cube Position", (float(&)[3])cubePosition, -1.f, 1.f);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Mouse position x: %.0f y: %.0f", mousePos.x, mousePos.y);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};