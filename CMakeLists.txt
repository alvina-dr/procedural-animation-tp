cmake_minimum_required (VERSION 3.5)

set(PROJECT_NAME OpenGLSandbox)
set(CMAKE_CXX_STANDARD 17)

project (${PROJECT_NAME})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
	message(FATAL_ERROR "the compiler ${CMAKE_CXX_COMPILER_ID} is incompatible with this project.")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  	message(FATAL_ERROR "the compiler ${CMAKE_CXX_COMPILER_ID} is incompatible with this project.")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	# using Visual Studio C++

	if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 22)
		set(MSVC_LIB_DIRNAME lib-vc2022)
	elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 19)
		set(MSVC_LIB_DIRNAME lib-vc2019)
	elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 17)
		set(MSVC_LIB_DIRNAME lib-vc2017)
	elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 15)
		set(MSVC_LIB_DIRNAME lib-vc2015)
	elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 13)
		set(MSVC_LIB_DIRNAME lib-vc2013)
	elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 12)
		set(MSVC_LIB_DIRNAME lib-vc2012)
	else()
		message(FATAL_ERROR "failed to determine msvc version.")
	endif()

	set(LINK_DIRECTORIES ./thirdparty/glfw/win64/${MSVC_LIB_DIRNAME})
	set(LINK_LIBRARIES opengl32.lib glfw3.lib)
	set(INCLUDE_DIRECTORIES ./thirdparty/glfw/win64/include)
else ()
	message(FATAL_ERROR "the compiler ${CMAKE_CXX_COMPILER_ID} is incompatible with this project.")
endif()

set(SOURCE_FILES 
	src/main.cpp
	src/myviewer.cpp
	src/boids/boidsviewer.cpp
	src/particles/particlesviewer.cpp
	src/forwardkinematic/fkviewer.cpp
	src/shader.cpp
	src/drawbuffer.cpp
	src/renderengine.cpp
	src/renderapi.cpp
	src/viewer.cpp
	thirdparty/glad/glad.c
	thirdparty/imgui/imgui.cpp
	thirdparty/imgui/imgui_demo.cpp
	thirdparty/imgui/imgui_draw.cpp
	thirdparty/imgui/imgui_tables.cpp
	thirdparty/imgui/imgui_widgets.cpp
	thirdparty/imgui/imgui_impl_glfw.cpp
	thirdparty/imgui/imgui_impl_opengl3.cpp
)

set(INCLUDE_DIRECTORIES 
	${INCLUDE_DIRECTORIES}
	thirdparty/glad
	thirdparty/glm
	thirdparty/imgui
)

file(REAL_PATH "./src/shaders/" SHADER_FILES_ABS_PATH)
add_compile_definitions(SHADER_PATH="${SHADER_FILES_ABS_PATH}/")

add_executable (${PROJECT_NAME} ${SOURCE_FILES})

target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(${PROJECT_NAME} PUBLIC ${LINK_DIRECTORIES})
target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

