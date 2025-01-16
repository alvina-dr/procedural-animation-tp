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

constexpr char const* FkViewerName = "FkViewer";
constexpr glm::vec4 FkWhite = { 1.f, 1.f, 1.f, 1.f };
constexpr glm::vec4 FkBlue = { 0.f, 0.f, 1.f, 1.f };
constexpr glm::vec4 FkGreen = { 0.f, 1.f, 0.f, 1.f };
constexpr glm::vec4 FkRed = { 1.f, 0.f, 0.f, 1.f };

struct FkVertexShaderAdditionalData {
	glm::vec3 Pos;
	/// beware of alignement (std430 rule)
};

class Joint {
public:
	Joint* ParentJoint = nullptr;
	Joint* ChildJoint = nullptr;
	//the relative position and rotation to the parent's joint
	glm::vec3 RPos;
	glm::vec3 REulRot;
	glm::quat RRot;
	//the absolute position and rotation
	glm::vec3 AbsPos;
	glm::quat AbsRot;

	Joint(Joint* parent) {
		ParentJoint = parent;
		RPos = glm::vec3(0, 0, 0);
		REulRot = glm::vec3(0, 0, 0);
		RRot = glm::angleAxis(0.f, glm::vec3(0, 0, 0));
	}
	Joint(Joint* parent, Joint* child) {
		ChildJoint = child;
	}
	Joint() {
		ParentJoint = new Joint(nullptr, this);
		ParentJoint->RPos = glm::vec3(0, 0, 0);
		ParentJoint->AbsPos = glm::vec3(0, 0, 0);
		ParentJoint->REulRot = glm::vec3(0, 0, 0);
		ParentJoint->AbsRot = glm::vec3(0, 0, 0);
		RPos = glm::vec3(0, 0, 0);
		REulRot = glm::vec3(0, 0, 0);
		RRot = glm::angleAxis(0.f, glm::vec3(0, 0, 0));
	}
	~Joint() {
		delete ParentJoint;
		ParentJoint = nullptr;
		delete ChildJoint;
		ChildJoint = nullptr;
	}
	void DrawFromParent(const RenderApi3D& api) {
		RRot = EulerToQuat(REulRot);
		AbsRot = ParentJoint->AbsRot * RRot;
		api.bone(RPos, FkWhite, AbsRot, ParentJoint->AbsPos);
		AbsPos = ParentJoint->AbsPos + (ParentJoint->AbsRot*(RRot * RPos));
		
		api.solidSphere(AbsPos, 0.05f, 10, 10, FkWhite);

		if (ChildJoint!=nullptr) {
			ChildJoint->DrawFromParent(api);
		}
	}

	void DrawGUI() {
		ImGui::PushID(this);
		if (ImGui::CollapsingHeader("Bone")) {
			if (ImGui::BeginTable("BoneParam", 3, ImGuiTableFlags_NoSavedSettings)) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::SliderAngle("##EulRRot X", &REulRot.x);
				ImGui::TableNextColumn();
				ImGui::SliderAngle("##EulRRot Y", &REulRot.y);
				ImGui::TableNextColumn();
				ImGui::SliderAngle("##EulRRot Z", &REulRot.z);
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Button("Reset X")) {
					REulRot.x = 0;
				}
				ImGui::TableNextColumn();
				if (ImGui::Button("Reset Y")) {
					REulRot.y = 0;
				}
				ImGui::TableNextColumn();
				if (ImGui::Button("Reset Z")) {
					REulRot.z = 0;
				}
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::SliderAngle("##RPos X", &RPos.x);
				ImGui::TableNextColumn();
				ImGui::SliderAngle("##RPos Y", &RPos.y);
				ImGui::TableNextColumn();
				ImGui::SliderAngle("##RPos Z", &RPos.z);
				ImGui::EndTable();
			}
		}
		ImGui::PopID();

	}

private:
	glm::quat EulerToQuat(glm::vec3 eulerRot) {
		glm::quat x = glm::angleAxis(eulerRot.x, glm::vec3(1.f, 0.f, 0.f));
		glm::quat y = glm::angleAxis(eulerRot.y, glm::vec3(0.f, 1.f, 0.f));
		glm::quat z = glm::angleAxis(eulerRot.z, glm::vec3(0.f, 0.f, 1.f));

		return x * y * z;
	}
};

struct FkViewer : Viewer {

	std::vector<Joint*> joints;

	float particleSize = 0.1f;
	int BoundsSize = 5.f;
	int ParticleVeloRandom = 5.f;
	int VoidStrgRandom = 5.f;

	//Inputs
	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;

	FkVertexShaderAdditionalData additionalShaderData;

	FkViewer() : Viewer(FkViewerName, 1280, 720) {}

	void init() override {
		//Inputs
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;

		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };

		joints.push_back(new Joint());
		joints.push_back(new Joint(joints[0]));
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
		CustomShaderDataSize = sizeof(FkVertexShaderAdditionalData);


		
	}

	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, glm::vec4(0.0f, 0.2f, 1.f, 1.f));
	}

	void render3D(const RenderApi3D& api) const override {
		/*api.horizontalPlane({ 0, 0, 0 }, { 10, 10 }, 1, glm::vec4(0.9f, 0.9f, 0.9f, 1.f));

		api.grid(10.f, 10, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);*/

		api.axisXYZ(nullptr);

		for (Joint* joint : joints)
		{
			joint->DrawFromParent(api);
		}
	}

	void render2D(const RenderApi2D& api) const override {

		constexpr float padding = 50.f;

		if (altKeyPressed) {
			if (leftMouseButtonPressed) {
				api.circleFill(mousePos, padding, 10, FkWhite);
			}
			else {
				api.circleContour(mousePos, padding, 10, FkWhite);
			}

		}
		else {
			const glm::vec2 min = mousePos + glm::vec2(padding, padding);
			const glm::vec2 max = mousePos + glm::vec2(-padding, -padding);
			if (leftMouseButtonPressed) {
				api.quadFill(min, max, FkWhite);
			}
			else {
				api.quadContour(min, max, FkWhite);
			}
		}

		{
			const glm::vec2 from = { viewportWidth * 0.5f, padding };
			const glm::vec2 to = { viewportWidth * 0.5f, 2.f * padding };
			constexpr float thickness = padding * 0.25f;
			constexpr float hatRatio = 0.3f;
			api.arrow(from, to, thickness, hatRatio, FkWhite);
		}

		{
			glm::vec2 vertices[] = {
				{ padding, viewportHeight - padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth - padding, viewportHeight - padding },
			};
			api.lines(vertices, COUNTOF(vertices), FkWhite);
		}
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("3D Sandbox");

		for (Joint* joint : joints)
		{
			joint->DrawGUI();
		}

		if (ImGui::CollapsingHeader("3D Sandbox param")) {
			ImGui::Checkbox("Show demo window", &showDemoWindow);

			ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);


			ImGui::SliderFloat("Point size", &pointSize, 0.1f, 10.f);
			ImGui::SliderFloat("Line Width", &lineWidth, 0.1f, 10.f);
			ImGui::Separator();
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