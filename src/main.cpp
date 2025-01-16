#include "myviewer.cpp"
#include "boids/boidsviewer.cpp"
#include "particles/particlesviewer.cpp"
#include "forwardkinematic/fkviewer.cpp"

int main(int argc, char** argv) {
	FkViewer v;
	return v.run();
}
