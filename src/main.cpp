#include "myviewer.cpp"
#include "boids/boidsviewer.cpp"
#include "boids/particlesviewer.cpp"
#include "boids/clothviewer.cpp"

int main(int argc, char** argv) {
	BoidsViewer v;
	return v.run();
}
