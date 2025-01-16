#include "myviewer.cpp"
#include "boids/boidsviewer.cpp"
#include "particles/particlesviewer.cpp"
#include "boids/clothviewer.cpp"
#include "forwardkinematic/fkviewer.cpp"

int main(int argc, char** argv) {
	ClothViewer v;
	return v.run();
}
