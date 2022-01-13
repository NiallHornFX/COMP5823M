// COMP5823M - A2 : Niall Horn - viewer.cpp
// Implements
#include "viewer.h"

// Std Headers
#include <iostream>
#include <vector>
#include <thread>

// Ext Headers
// GLEW
#include "ext/GLEW/glew.h" 
// GLFW
#include "ext/GLFW/glfw3.h" 
// GLM
#include "ext/glm/gtc/matrix_transform.hpp"
#include "ext/glm/gtc/type_ptr.hpp"
// dearimgui
#include "ext/dearimgui/imgui.h"
#include "ext/dearimgui/imgui_impl_glfw.h"
#include "ext/dearimgui/imgui_impl_opengl3.h"

// Project Headers
#include "fluid_object.h"
#include "fluid_solver.h"

#define USE_FREE_CAMERA 1

// Global GLFW State
struct
{
	int    width, height;
	double mouse_offset_x  = 0.f, mouse_offset_y  = 0.f;
	double mousepos_prev_x = 0.f, mousepos_prev_y = 0.f;
	double scroll_y = 0.f;
	bool is_init = false; 
}GLFWState;

// =========================================== Viewer Class Implementation ===========================================

Viewer::Viewer(std::size_t W, std::size_t H, const char *Title)
	: width(W), height(H), title(Title)
{
	// ============= Init =============
	tick_c = 0;
	draw_axis = false;

	// ============= OpenGL Setup ============= 
	// Setup OpenGL Context and Window
	window_context(); 
	// Load OpenGL Extensions
	extensions_load();
	// Camera
	ortho = glm::ortho(0.f, 10.f, 0.f, 10.f);

	// ============= Fluid Setup =============
	fluid_object = new Fluid_Object;
	fluid_solver = new Fluid_Solver((1.f / 196.f), 100.f, 0.5f, fluid_object);

	// Get Neighbours Initally for Hash Debug
	fluid_solver->get_neighbours();
}

Viewer::~Viewer() 
{
	glfwDestroyWindow(window); window = nullptr;
	glfwTerminate();
}

// Initalizes viewer state and calls indefinite application execution loop.
void Viewer::exec()
{
	// ============= Init Operations =============
	render_prep();

	// ============= Application Loop =============
	bool esc = false; 
	while (!glfwWindowShouldClose(window) && !esc)
	{
		// Tick viewer application
		tick();

		// Query Esc key
		esc = esc_pressed();
	} 

	// ============= Shutdown GUI =============
	gui_shutdown();
}


// Single tick of the viewer application, all runtime operations are called from here. 
void Viewer::tick()
{
	// ============= App Operations =============
	get_dt();
	update_window();

	// ============= Input Query =============

	// ============= Simulation =============
	fluid_solver->tick(dt);

	// ============= Render =============
	render();

	// ============= Post Tick Operations =============
	tick_c++;
}

// Create Window via GLFW and Initalize OpenGL Context on current thread. 
void Viewer::window_context()
{
	// ============= GLFW Setup =============
	glfwInit();
	if (!glfwInit())
	{
		std::cerr << "Error::Viewer:: GLFW failed to initalize.\n";
		std::terminate();
	}
	// Window State
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // Fixed Window Size. 
	glfwWindowHint(GLFW_SAMPLES, 4); // MSAA.
	// Create Window
	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Error::Viewer:: GLFW failed to initalize.\n";
		glfwTerminate();
		std::terminate();
	}

	// ============= Set GLFW Callbacks =============
	// Window Callack
	glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);
	// Mouse Callbacks
	glfwSetCursorPosCallback(window, &mouse_callback);
	glfwSetScrollCallback(window, &scroll_callback);

	// ============= Set Context and Viewport =============
	glfwMakeContextCurrent(window);
	glViewport(0, 0, width, height);

	// ============= Setup GUI =============
	gui_setup();
}

// Load OpenGL Functions via GLEW and output device info.
void Viewer::extensions_load()
{
	// GLEW Setup
	glewExperimental = GL_TRUE;
	glewInit();
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "ERROR::Viewer:: GLFW failed to initalize.\n";
		std::terminate();
	}

	// Query GL Device and Version Info - 
	render_device = glGetString(GL_RENDERER);
	version = glGetString(GL_VERSION);
	// Cleanup Debug Output
	std::cout << "======== DEBUG::OPENGL::BEGIN ========\n"
		<< "RENDER DEVICE = " << render_device << "\n"
		<< "VERSION = " << version << "\n";
	std::cout << "======== DEBUG::OPENGL::END ========\n\n";
}

// Initalize Render State
void Viewer::render_prep()
{
	// ============= OpenGL Pre Render State =============
	// Multisampling 
	glEnable(GL_MULTISAMPLE);

	// Default glPrim Sizes
	glPointSize(5.f);
	glLineWidth(2.5f);

	// Blending and Depth. 
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ============= Create Viewer Primtivies =============
	// Axis
	axis = new Primitive("axis");
	float data[44] =
	{
		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f,
		0.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f,
		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
		0.f, 0.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f
	};
	axis->set_data_mesh(data, 6);
	axis->set_shader("../../shaders/basic.vert", "../../shaders/basic.frag");
	axis->mode = Render_Mode::RENDER_LINES;
}

// Render Operations
void Viewer::render()
{
	// ==================== Render State ====================
	glClearColor(0.1f, 0.1f, 0.1f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ==================== Render Viewer Primtivies ====================
	// Draw Axis
	if (draw_axis)
	{
		axis->line_width = 2.5f; // Reset for axis width.
		axis->render();
	}
	// ==================== Render Fluid ====================
	fluid_object->render(ortho);

	// ==================== Render Fluid Colliders ====================
	fluid_solver->render_colliders(ortho);

	// ==================== Render GUI ====================
	gui_render();

	// ====================  Swap and Poll ====================
	get_GLError();
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void Viewer::query_drawState()
{
	// 
}

void Viewer::update_window()
{
	// Nth frame update
	if (tick_c % 5 != 0) return;

	// Update Window Title 
	std::string title_u;
	title_u = title + "      OpenGL " + std::to_string(GL_MAJOR) + "." + std::to_string(GL_MINOR)
		+ "       FPS : " + std::to_string(1.f / dt);
	
	glfwSetWindowTitle(window, title_u.c_str());
}

void Viewer::get_GLError()
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) std::cerr << "ERROR::Viewer::GL_ERROR = " << err << std::endl;
}

void Viewer::get_dt()
{
	prev_t = cur_t; 
	cur_t = glfwGetTime();
	dt = cur_t - prev_t; 
}

bool Viewer::esc_pressed()
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) return true;
	return false; 
}


// =========================================== DearImGUI Implementation ===========================================

// Info : imgui Startup 
void Viewer::gui_setup()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 400");
}

// Info : GUI Render with forwarding the relveant data based on input.
void Viewer::gui_render()
{
	get_GLError();
	bool window = true; 

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ============= GUI Static Locals / State =============
	// Solver State Text
	std::string state; 
	if (fluid_solver->simulate) state = "Solve Running"; else state = "Solve Stopped";

	// Fluid Object Core parameters (Reconstruct if changed)
	static float spc = DEF_SPC;
	static float pos [2] = { DEF_XP, DEF_YP };
	static float dim [2] = { DEF_XS, DEF_YS };

	// Fluid Solver Core parameters (Reconstruct if changed)
	static float kernel_radius = 0.5; 

	// Get Dt 1/n. 
	float n = 1.f / fluid_solver->dt;
	static int tmp_count = 90;


	// ============= Imgui layout =============
	{
		// Begin ImGui
		ImGui::Begin("Simulation Controls");

		// ========== Solver State ==========
		// Labels
		if (fluid_solver->simulate) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255)); else ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
		ImGui::Text(state.c_str());
		ImGui::Text("Simulation Frame = %d, Substep = %d", fluid_solver->frame, fluid_solver->timestep);
		ImGui::Text("Dt = 1/%d", std::size_t(n));
		ImGui::PopStyleColor();

		// Anim Loop Play Pause
		if (ImGui::Button("Start/Stop"))
		{
			fluid_solver->simulate = !fluid_solver->simulate;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		// Reset
		if (ImGui::Button("Reset"))
		{
			fluid_solver->simulate = false;
			fluid_solver->reset();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// ========== Fluid Attributes  ==========
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("Fluid Attributes");
		ImGui::Text("Particle Count = %d", fluid_object->particles.size());
		ImGui::PopStyleColor();
		ImGui::Text("Mass : %f", fluid_object->particles[0].mass);
		ImGui::Text("Density : min = %f | max = %f",  fluid_solver->min_dens, fluid_solver->max_dens);
		ImGui::Text("Pressure : min = %f | max = %f", fluid_solver->min_pres, fluid_solver->max_pres);
		//ImGui::Text("Forcesqr : min = %f | max = %f", fluid_solver->min_force, fluid_solver->max_force);

		// ========== Fluid State Controls ==========
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("Fluid State Controls");
		ImGui::PopStyleColor();

		// Causes rebuild of Fluid_Object if changed (could just reset ideally)
		if (ImGui::SliderFloat2("Fluid Pos", pos, 0.f, 10.f))
		{
			fluid_solver->simulate = false; 
			delete fluid_object;
			fluid_object = new Fluid_Object(glm::vec2(pos[0], pos[1]), glm::vec2(dim[0], dim[1]), spc);
			fluid_solver->fluidData = fluid_object;
		}
		if (ImGui::SliderFloat2("Fluid Dim", dim, 0.f, 10.f))
		{
			fluid_solver->simulate = false;
			delete fluid_object;
			fluid_object = new Fluid_Object(glm::vec2(pos[0], pos[1]), glm::vec2(dim[0], dim[1]), spc);
			fluid_solver->fluidData = fluid_object;
		}
		if (ImGui::SliderFloat("Fluid Spc", &spc, 0.0001f, 0.5f))
		{
			fluid_solver->simulate = false;
			Fluid_Object::Colour_Viz old_pc = fluid_object->particle_colour;
			delete fluid_object;
			fluid_object = new Fluid_Object(glm::vec2(pos[0], pos[1]), glm::vec2(dim[0], dim[1]), spc);
			fluid_object->particle_colour = old_pc;
			fluid_solver->fluidData = fluid_object;

			// Get Neighbours Initally for Hash Debug
			fluid_solver->get_neighbours();
		}

		// Causes rebuild of Fluid_Solver if changed (this is so kernel coeffs can be pre-computed) 
		if (ImGui::SliderFloat("Kernel Radius", &kernel_radius, 0.1f, 2.f))
		{
			fluid_solver->simulate = false;
			// Store old data 
			float dt = fluid_solver->dt, rest_dens = fluid_solver->rest_density, stiff = fluid_solver->stiffness_coeff, g = fluid_solver->gravity, ar = fluid_solver->air_resist;
			delete fluid_solver;
			// Alloc solver with updated kernel size
			fluid_solver = new Fluid_Solver(dt, rest_dens, kernel_radius, fluid_object);
			fluid_solver->stiffness_coeff = stiff, fluid_solver->gravity = g, fluid_solver->air_resist = ar;
			// Calc Rest_Density.
			//fluid_solver->calc_restdens();
		}

		// Free parameters 
		ImGui::SliderFloat("Rest Dens", &fluid_solver->rest_density,    1.f, 1000.f);
		ImGui::SliderFloat("Stiffness", &fluid_solver->stiffness_coeff, 0.f, 1000.f);
		ImGui::SliderFloat("Gravity"  , &fluid_solver->gravity,        -10.f, 10.f);
		ImGui::SliderFloat("AirResist", &fluid_solver->air_resist,       0.f, 5.f);

		// ========== Fluid Rendering ==========
		// Draw Axis
		if (ImGui::Button("Draw Origin Axis"))
		{
			draw_axis = !draw_axis; 
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Particle Colours 
		if (ImGui::Button("Colour : Const"))
		{
			fluid_object->particle_colour = Fluid_Object::Colour_Viz::Standard;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if (ImGui::Button("Colour : Pressure"))
		{
			fluid_object->particle_colour = Fluid_Object::Colour_Viz::Pressure;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if (ImGui::Button("Colour : Density"))
		{
			fluid_object->particle_colour = Fluid_Object::Colour_Viz::Density;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if (ImGui::Button("Colour : Cells"))
		{
			fluid_object->particle_colour = Fluid_Object::Colour_Viz::GridCell;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// End ImGui
		ImGui::End();
	}

	// ============= Imgui Render =============
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	get_GLError();
}

// Info : imgui shut down. 
void Viewer::gui_shutdown()
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

// =========================================== GLFW State + Callbacks ===========================================

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	GLFWState.width = width, GLFWState.height = height; 
	glViewport(0, 0, GLFWState.width, GLFWState.height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (!GLFWState.is_init)
	{
		GLFWState.mousepos_prev_x = xpos;
		GLFWState.mousepos_prev_y = ypos;
		GLFWState.is_init = true;
	}
	// Mouse Offset
	GLFWState.mouse_offset_x =  (xpos - GLFWState.mousepos_prev_x);
	GLFWState.mouse_offset_y =  (ypos - GLFWState.mousepos_prev_y);

	// Prev Pos
	GLFWState.mousepos_prev_x = xpos;
	GLFWState.mousepos_prev_y = ypos;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	GLFWState.scroll_y = yoffset;
}

