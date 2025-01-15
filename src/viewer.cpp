#include "viewer.h"

#define GLFW_INCLUDE_NONE

#include "shader.h"
#include "drawbuffer.h"
#include "renderengine.h"
#include "camera.h"

#include <time.h>

#include <GLFW/glfw3.h>
#include <glad.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define COUNTOF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))


namespace {
	void GLAPIENTRY MessageCallback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		if (type != GL_DEBUG_TYPE_ERROR)
			return;
		fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
			type, severity, message);
		if (severity == GL_DEBUG_SEVERITY_HIGH) {
			assert(false);
		}
	}

	bool checkOpenGlError() {
		
		int error;
		if ((error = glGetError()) != GL_NO_ERROR) {
			char const* errorString;
			switch (error)
			{
			case GL_INVALID_ENUM:
				errorString = "GL_INVALID_ENUM";
				break;
			case GL_INVALID_VALUE:
				errorString = "GL_INVALID_VALUE";
				break;
			case GL_INVALID_OPERATION:
				errorString = "GL_INVALID_OPERATION";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
				break;
			case GL_OUT_OF_MEMORY:
				errorString = "GL_OUT_OF_MEMORY";
				break;
			default:
				errorString = "UNKNOWN";
				break;
			}
			fprintf(stdout, "OpenGL Error: %s\n", errorString);
			assert(false);
		}
		return error != GL_NO_ERROR;
	}

	struct GUIStates {
		bool panLock;
		bool turnLock;
		bool zoomLock;
		int lockPositionX;
		int lockPositionY;

		static constexpr float GUIStates::MOUSE_PAN_SPEED = 0.001f;
		static constexpr float GUIStates::MOUSE_ZOOM_SPEED = 0.005f;
		static constexpr float GUIStates::MOUSE_ZOOM_SCROLL_SPEED = 10.f * GUIStates::MOUSE_ZOOM_SPEED;
		static constexpr float GUIStates::MOUSE_TURN_SPEED = 0.005f;
	};

	void initGUIStates(GUIStates& guiStates) {
		guiStates.panLock = false;
		guiStates.turnLock = false;
		guiStates.zoomLock = false;
		guiStates.lockPositionX = 0;
		guiStates.lockPositionY = 0;
	}

	void render3DCallback(const RenderApi3D& api, void* pUserData) {
		const Viewer& viewer = *reinterpret_cast<Viewer const*>(pUserData);
		viewer.render3D(api);
	}

	void render3DCustomCallback(const RenderApi3D& api, void* pUserData) {
		const Viewer& viewer = *reinterpret_cast<Viewer const*>(pUserData);
		viewer.render3D_custom(api);
	}
	void render2DCallback(const RenderApi2D& api, void* pUserData) {
		const Viewer& viewer = *reinterpret_cast<Viewer const*>(pUserData);
		viewer.render2D(api);
	}
}

Viewer::Viewer(char const* initialWindowName, int initialViewportWidth, int initialViewportHeight) {
	strncpy(windowName, initialWindowName, COUNTOF(windowName));
	cameraCreate(camera);

	pointSize = 1.f;
	lineWidth = 1.f;
	backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);

	lightDir = glm::vec4(0.1f, 0.3f, 0.2f, 1.f);
	lightStrength = 0.65f;
	lightAmbient = 0.16f;
	specular = 0.15f;
	specularPow = 30.f;


	viewportWidth = initialViewportWidth;
	viewportHeight = initialViewportHeight;

	window = nullptr;

	pCustomShaderData = nullptr;
	CustomShaderDataSize = 0;
}

namespace {
	void windowScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
		Viewer* pViewer = reinterpret_cast<Viewer*>(glfwGetWindowUserPointer(window));
		assert(pViewer);
		cameraZoom(pViewer->camera, float(-yoffset) * GUIStates::MOUSE_ZOOM_SCROLL_SPEED);
	}
}

int /*exit code*/ Viewer::run() {

	// Initialize glfw library
	if (!glfwInit()) {
		fprintf(stderr, "Failed to init glfw");
		system("pause");
		return -1;
	}

#define ERROR(_ERROR_)\
  glfwTerminate();\
  fprintf(stderr, #_ERROR_);\
  fprintf(stderr, "\n");\
  system("pause");\
  return -1;

	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
	glfwWindowHint(GLFW_DECORATED, GL_TRUE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

#if defined(__APPLE__)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
#endif

	double t;
	double fps = 0.0;

	// Create a windowed mode window and its OpenGL context
	window = glfwCreateWindow(viewportWidth, viewportHeight, windowName, NULL, NULL);
	if (!window) {
		ERROR("Failed to create window");
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Create OpenGL context
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		ERROR("Failed to initialize OpenGL context.");
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glfwSetWindowUserPointer(window, this);
	glfwSetScrollCallback(window, windowScrollCallback);

	//-- Debg callback
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	// Init gui structures
	GUIStates guiStates;
	initGUIStates(guiStates);

	// Init render engine
	RenderEngine renderEngine;
	if (!createRenderEngine(renderEngine)) {
		ERROR("Failed to create render engine");
	}

	// call virtual method
	init();

	if (checkOpenGlError()) {
		ERROR("OpenGL Error before launching main loop");
	}

	const clock_t startTime = clock();

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(window) && (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS)) {
		t = glfwGetTime();

		// Poll for and process events
		glfwPollEvents();

		glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);

		// Mouse states
		int leftButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		int rightButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
		int middleButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);

		if (leftButton == GLFW_PRESS) {
			guiStates.turnLock = true;
		}
		else {
			guiStates.turnLock = false;
		}

		if (rightButton == GLFW_PRESS) {
			guiStates.zoomLock = true;
		}
		else {
			guiStates.zoomLock = false;
		}

		if (middleButton == GLFW_PRESS) {
			guiStates.panLock = true;
		}
		else {
			guiStates.panLock = false;
		}

		// Camera movements
		int leftAltPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT);
		int rightAltPressed = glfwGetKey(window, GLFW_KEY_RIGHT_ALT);
		int f7Pressed = glfwGetKey(window, GLFW_KEY_F7);

		const bool altPressed = leftAltPressed == GLFW_PRESS || rightAltPressed == GLFW_PRESS;

		double mousex_d, mousey_d;
		glfwGetCursorPos(window, &mousex_d, &mousey_d);
		int mousex = (int)mousex_d;
		int mousey = (int)mousey_d;

		if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS)) {
			guiStates.lockPositionX = mousex;
			guiStates.lockPositionY = mousey;
		}
		if (altPressed) {
			int diffLockPositionX = mousex - guiStates.lockPositionX;
			int diffLockPositionY = mousey - guiStates.lockPositionY;
			if (guiStates.zoomLock) {
				float zoomDir = 0.0;
				if (diffLockPositionX > 0) {
					zoomDir = -1.f;
				}
				else if (diffLockPositionX < 0) {
					zoomDir = 1.f;
				}
				cameraZoom(camera, diffLockPositionX * GUIStates::MOUSE_ZOOM_SPEED);
			}
			else if (guiStates.turnLock) {
				cameraTurn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED, diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);
			}
			else if (guiStates.panLock) {
				cameraPan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED, diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
			}
			guiStates.lockPositionX = mousex;
			guiStates.lockPositionY = mousey;
		}

		if (f7Pressed) {
			reloadRenderEngineShaders(renderEngine);
		}

		const clock_t currentTime = clock();
		const double elapsedTime = (currentTime - startTime) / double(CLOCKS_PER_SEC);
		update(elapsedTime);

		RenderParams renderParams;
		renderParams.render3DCallback = render3DCallback;
		renderParams.pRender3DCallbackUserData = this;

		renderParams.render3DCustomCallback = render3DCustomCallback;
		renderParams.pRender3DCustomCallbackUserData = this;

		renderParams.render2DCallback = render2DCallback;
		renderParams.pRender2DCallbackUserData = this;

		renderParams.pCamera = &camera;

		renderParams.backgroundColor = backgroundColor;

		renderParams.pointSize = pointSize;
		renderParams.lineWidth = lineWidth;

		renderParams.lightDirection = lightDir;
		renderParams.lightStrength= lightStrength; 
		renderParams.lightAmbient = lightAmbient;
		renderParams.specular = specular;
		renderParams.specularPow = specularPow;


		renderParams.viewportWidth = viewportWidth;
		renderParams.viewportHeight = viewportHeight;

		renderParams.time = (float)t;
		renderParams.pCustomVertShaderData = pCustomShaderData;
		renderParams.CustomVertShaderDataSize = CustomShaderDataSize;

		renderEngineFrame(renderEngine, renderParams);

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		drawGUI();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, viewportWidth, viewportHeight);
		//glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap front and back buffers
		glfwSwapBuffers(window);

		if (checkOpenGlError()) {
			assert(false);
		}

		double newTime = glfwGetTime();
		fps = 1.0 / (newTime - t);

		char windowNameEx[COUNTOF(windowName) * 2];
		sprintf(windowNameEx, "%s - %.0f fps", windowName, fps);
		glfwSetWindowTitle(window, windowNameEx);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}