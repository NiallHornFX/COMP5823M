// COMP5823M - A1 : Niall Horn - main.cpp

// Project Headers
#include "viewer.h"

// Test
#include "cloth_state.h"

int main(int argc, char **argv)
{
	// Create Viewer Application Instance
	Viewer app(1024, 1024, "COMP5823M_A2 Cloth Solver");

	// Exec
	app.exec();

	return 0; 
}

