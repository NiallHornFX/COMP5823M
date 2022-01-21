// COMP5823M - A3 : Niall Horn - viewer.h
#ifndef VIEWER_H
#define VIEWER_H

// Std Includes
#include <string>

// Project Headers
#include "primitive.h"

// Ext Headers
#include "ext/glm/glm.hpp"

// Typedef 
using byte = unsigned char;

// OpenGL Version
#define GL_MAJOR 4
#define GL_MINOR 3

// FD 
class Fluid_Object;
class Fluid_Solver; 
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

	// State Query
	void query_drawState();
	bool esc_pressed();
	void get_dt();

	// Debug
	void get_GLError();

private:
	// Render State
	GLFWwindow *window; 
	std::size_t width, height;
	std::string title; 
	bool draw_axis;
	bool ren_pts, ren_meta; 

	const byte *render_device;
	const byte *version;

	// Camera
	glm::mat4 ortho;

	// Fluid Simulation
	Fluid_Object *fluid_object; 
	Fluid_Solver *fluid_solver; 

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