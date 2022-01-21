// COMP5823M A2 : Niall Horn 201486968 - viewer.h
#ifndef VIEWER_H
#define VIEWER_H

// Std Includes
#include <string>

// Project Headers
#include "camera.h"
#include "primitive.h"
#include "ground.h"
#include "mesh.h"
#include "cloth_state.h"
#include "cloth_solver.h"

// Ext Headers
#include "ext/glm/glm.hpp"

// Typedef 
using byte = unsigned char;

// OpenGL Version
#define GL_MAJOR 4
#define GL_MINOR 0

// FD 
struct GLFWwindow;
struct GLFWState;

// Viewer application for rendering, GUI and handling user input.

class Viewer
{
public:
	Viewer() = delete; 
	Viewer(std::size_t W, std::size_t H, const char *Title);
	~Viewer(); 

	// OpenGL Setup
	void window_context();
	void extensions_load();

	// Rendering
	void render_prep();
	void render();

	// GUI
	void gui_setup();
	void gui_render();
	void gui_shutdown();

	// Application 
	void exec(); // Exec Viewer Application 
	void tick(); // Single Tick

	// Per Tick Operations
	void update_window();
	void update_camera();

	// State Query
	void query_drawState();
	bool esc_pressed();
	void get_dt();

	// Debug
	void test_mesh();

private:
	void get_GLError();

private:
	// Render State
	GLFWwindow *window; 
	std::size_t width, height;
	std::string title; 
	bool draw_grid, draw_axis;

	const byte *render_device;
	const byte *version;

	// Cloth Simulation
	Cloth_State    *cloth; 
	Cloth_Solver   *cloth_solver;
	Cloth_Collider *collision_plane, *collision_sphere; 

	// Camera 
	Camera camera; 
	float last_yawoffs, last_pitchoffs, last_zoom;

	// Light
	glm::vec3 light_pos; 
	float light_strength; 
	bool ren_normals; 
	bool ren_wire; 

	// Primtivies
	std::vector<Primitive*> prims;
	Ground *ground;
	Primitive *axis;

	// Viewer Intrinsics
	std::size_t tick_c; 
	float dt;
	float cur_t, prev_t;
};

// GLFW Input Callbacks 
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

#endif