#ifndef VIEWER_H
#define VIEWER_H

// Std Includes
#include <string>

// Project Headers
#include "camera.h"
#include "primitive.h"
#include "ground.h"
#include "mesh.h"
#include "anim_state.h"

// Ext Headers
#include "ext/glm/glm.hpp"

// Typedef 
using byte = unsigned char;

// OpenGL Version
#define GL_MAJOR 4
#define GL_MINOR 3

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
	//
	void query_drawState();

	// Application 
	void exec(); // Exec Viewer Application 
	void tick(); // Single Tick

	// Per Tick Operations
	void update_window();
	void update_camera();

	// State Query
	bool esc_pressed();
	void get_dt();

	//void get_animData(); 

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

	// Animation State (Handles all animation realted tasks)
	Anim_State anim;

	// Camera 
	Camera camera; 
	float last_yawoffs, last_pitchoffs, last_zoom;

	// Primtivies
	std::vector<Primitive*> prims;
	Ground *ground;
	Primitive *axis;

	// Debugging
	Bone *bone_test;
	Skeleton *skel;

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