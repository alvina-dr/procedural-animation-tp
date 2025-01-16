#include "myviewer.cpp"
#include "boids/boidsviewer.cpp"
#include "boids/particlesviewer.cpp"

int main(int argc, char** argv) {
	ParticlesViewer v;
	return v.run();
}
