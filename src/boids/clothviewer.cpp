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
#define CONSTRAINT_ITERATIONS 15 // how many iterations of constraint satisfaction each frame (more is rigid, less is soft)
#define DAMPING 0.01f // how much to damp the cloth simulation each frame
#define TIME_STEPSIZE2 0.5f*0.5f // how large time step each particle takes each frame

constexpr char const* clothViewerName = "ClothViewer";

class ClothParticle 
{
public:
	bool CanMove;
	glm::vec3 Position;
	glm::vec3 OldPosition;
	glm::vec3 Velocity;
	float Mass = 1; // the mass of the particle (is always 1 in this example)

	ClothParticle(glm::vec3 position) : Position(position), CanMove(true), OldPosition(position), Velocity(glm::vec3(0, 0, 0)) { }

	void offsetPos(const glm::vec3 v) { if (CanMove) Position += v; }

	void makeUnmovable() { CanMove = false; }
	void resetAcceleration() { Velocity = glm::vec3(0, 0, 0); }

	void addForce(glm::vec3 f)
	{
		Velocity += f / Mass;
	}

	// This is one of the important methods, where the time is progressed a single step size (TIME_STEPSIZE)
	// The method is called by Cloth.time_step()
	// Given the equation "force = mass * acceleration" the next position is found through verlet integration
	void timeStep()
	{
		if (CanMove)
		{
			glm::vec3 temp = Position;
			Position = Position + (Position - OldPosition) * (1.0f - DAMPING) + Velocity * TIME_STEPSIZE2;
			OldPosition = temp;
			Velocity = glm::vec3(0, 0, 0); // acceleration is reset since it HAS been translated into a change in position (and implicitely into velocity)	
		}
	}

	void updateDerivatives(float dt)
	{
		Velocity = (Position - OldPosition) / dt;
	}
};

class Constraint
{
private:
	float restDistance; // the length between particle p1 and p2 in rest configuration
	float strength;

public:
	ClothParticle* p1, * p2; // the two particles that are connected through this constraint

	Constraint(ClothParticle* p1, ClothParticle* p2) : p1(p1), p2(p2)
	{
		restDistance = distance(p1->Position, p2->Position);
	}

	/* This is one of the important methods, where a single constraint between two particles p1 and p2 is solved
	the method is called by Cloth.time_step() many times per frame*/
	void satisfyConstraint()
	{
		glm::vec3 p1_to_p2 = p2->Position - p1->Position; // vector from p1 to p2
		float current_distance = distance(p1->Position, p2->Position); // current distance between p1 and p2
		glm::vec3 correctionVector = p1_to_p2 * (1 - restDistance / current_distance); // The offset vector that could moves p1 into a distance of rest_distance to p2
		glm::vec3 correctionVectorHalf = glm::vec3(correctionVector.x * 0.5, correctionVector.y * 0.5, correctionVector.z * 0.5); // Lets make it half that length, so that we can move BOTH p1 and p2.
		p1->offsetPos(correctionVectorHalf); // correctionVectorHalf is pointing from p1 to p2, so the length should move p1 half the length needed to satisfy the constraint.
		p2->offsetPos(-correctionVectorHalf); // we must move p2 the negative direction of correctionVectorHalf since it points from p2 to p1, and not p1 to p2.	
	}
};

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

	// Tweakable data
	int clothWidth = 10;
	int clothHeight = 10;
	float width = 5;
	float height = 5;

	glm::vec3 windForce = glm::vec3(0, 0, 0);
	glm::vec3 gravity = glm::vec3(0, 0, 0);

	float oldElapsedTime;

	std::vector<ClothParticle*> particleList; // all particles that are part of this cloth
	std::vector<Constraint*> constraintList; // alle constraints between particles as part of this cloth

	ClothParticle* getParticle(int x, int y) { return particleList[y * clothWidth + x]; }
	void makeConstraint(ClothParticle* p1, ClothParticle* p2) { constraintList.push_back(new Constraint(p1, p2)); }

	ClothViewer() : Viewer(clothViewerName, 1280, 720) {}

	static float distance(glm::vec3 position1, glm::vec3 position2) {
		float xSqr = (position1.x - position2.x) * (position1.x - position2.x);
		float ySqr = (position1.y - position2.y) * (position1.y - position2.y);
		float zSqr = (position1.z - position2.z) * (position1.z - position2.z);

		float sqr = xSqr + ySqr + zSqr;

		return sqrt(sqr);
	}

	void deleteRandomConstraint() 
	{
		int index = rand() % clothWidth* clothHeight;
		for (int i = 0; i < constraintList.size(); i++) 
		{
			if (index == i)
				constraintList.erase(constraintList.begin() + i);
		}
	}

	/* this is an important methods where the time is progressed one time step for the entire cloth.
	This includes calling satisfyConstraint() for every constraint, and calling timeStep() for all particles
	*/
	void timeStep()
	{
		for (int i = 0; i < CONSTRAINT_ITERATIONS; i++) // iterate over all constraints several times
		{
			for (size_t i = 0; i < constraintList.size(); i++)
			{
				constraintList[i]->satisfyConstraint(); // satisfy constraint.
			}
		}

		std::vector<Particle>::iterator particle;
		for (size_t i = 0; i < particleList.size(); i++)
		{
			particleList[i]->timeStep(); // calculate the position of each particle at the next time step.
		}
	}

	void addClothForce(glm::vec3 force) 
	{
		for (size_t i = 0; i < particleList.size(); i++)
		{
			particleList[i]->addForce(force);
		}
	}

	void applyAirFriction()
	{
		const float friction_coef = 0.5f;
		for (size_t i = 0; i < particleList.size(); i++)
		{
			particleList[i]->Velocity *= -friction_coef;
		}
	}

	void initCloth() 
	{
		constraintList.clear();

		particleList.clear();
		particleList.resize(clothWidth * clothHeight); //I am essentially using this vector as an array with room for num_particles_width*num_particles_height particles

		// Creating particles in a grid of particles from (0,0,0) to (width,-height,0)
		for (int x = 0; x < clothWidth; x++)
		{
			for (int y = 0; y < clothHeight; y++)
			{
				glm::vec3 pos = glm::vec3(width * (x / (float)clothWidth), height * (y / (float)clothHeight), 0);
				particleList[y * clothWidth + x] = new ClothParticle(pos); // insert particle in column x at y'th row
			}
		}

		// Connecting immediate neighbor particles with constraints (distance 1 and sqrt(2) in the grid)
		for (int x = 0; x < clothWidth; x++)
		{
			for (int y = 0; y < clothHeight; y++)
			{
				if (x < clothWidth - 1) makeConstraint(getParticle(x, y), getParticle(x + 1, y));
				if (y < clothHeight - 1) makeConstraint(getParticle(x, y), getParticle(x, y + 1));

				// Uncomment to add diagonal neighbors
				//if (x < clothWidth - 1 && y < clothHeight - 1) makeConstraint(getParticle(x, y), getParticle(x + 1, y + 1));
				//if (x < clothWidth - 1 && y < clothHeight - 1) makeConstraint(getParticle(x + 1, y), getParticle(x, y + 1));
			}
		}

		// Connecting secondary neighbors with constraints (distance 2 and sqrt(4) in the grid)
		//for (int x = 0; x < clothWidth; x++)
		//{
		//	for (int y = 0; y < clothHeight; y++)
		//	{
		//		if (x < clothWidth - 2) makeConstraint(getParticle(x, y), getParticle(x + 2, y));
		//		if (y < clothHeight - 2) makeConstraint(getParticle(x, y), getParticle(x, y + 2));
		//		if (x < clothWidth - 2 && y < clothHeight - 2) makeConstraint(getParticle(x, y), getParticle(x + 2, y + 2));
		//		if (x < clothWidth - 2 && y < clothHeight - 2) makeConstraint(getParticle(x + 2, y), getParticle(x, y + 2));
		//	}
		//}

		// Making the upper left most three and right most three particles unmovable
		for (int i = 0; i < 2; i++)
		{
			getParticle(0 + i, 0)->makeUnmovable();
			getParticle(clothWidth - 1 - i, 0)->makeUnmovable();
		}
	}

	void init() override 
	{
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;

		altKeyPressed = false;

		initCloth();
	}

	void update(double elapsedTime) override
	{
		leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		altKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		mousePos = { float(mouseX), viewportHeight - float(mouseY) };

		// Delta time
		float deltaTime = elapsedTime - oldElapsedTime;
		oldElapsedTime = elapsedTime;

		float random = (float)(rand() % 10) * 0.01f;
		glm::vec3 force = random * windForce;
		addClothForce(gravity);
		addClothForce(force);
		applyAirFriction();
		timeStep();
	}

	void render3D_custom(const RenderApi3D& api) const override
	{
		//Here goes your drawcalls affected by the custom vertex shader
	}

	void render3D(const RenderApi3D& api) const override {
		//api.horizontalPlane({ 0, 0, 0 }, { 10, 10 }, 1, glm::vec4(0.9f, 0.9f, 0.9f, 1.f));
		//api.grid(10.f, 10, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);
		//api.axisXYZ(nullptr);

		for (size_t i = 0; i < particleList.size(); i++)
		{
			api.solidSphere(particleList[i]->Position, 0.08f, 10, 10, boidsGreen);
		}

		for (size_t i = 0; i < constraintList.size(); i++)
		{
			glm::vec3 vertices[2] =
			{
				constraintList[i]->p1->Position,
				constraintList[i]->p2->Position
			};
			api.lines(vertices, 2, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);
		}
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
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("3D Sandbox");

		ImGui::Checkbox("Show demo window", &showDemoWindow);
		ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);
		ImGui::Separator();
		ImGui::SliderFloat3("Gravity", &gravity.x, -1.0f, 1.0f);
		ImGui::SliderFloat3("Wind Force", &windForce.x, -3.0f, 3.0f);
		if (ImGui::Button("Erase random constraint")) 
		{
			deleteRandomConstraint();
		}

		if (ImGui::Button("New Cloth"))
		{
			initCloth();
		}
		
		ImGui::Separator();

		float fovDegrees = glm::degrees(camera.fov);
		if (ImGui::SliderFloat("Camera field of fiew (degrees)", &fovDegrees, 15, 180)) 
		{
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