// COMP5823M - A3 : Niall Horn - viewer.cpp
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
#include "spatial_grid.h"

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
	fluid_solver = new Fluid_Solver((1.f / 196.f), 0.5f, fluid_object);
	fluid_object->solver = fluid_solver; 
	ren_pts = true, ren_meta = true; 
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
	//

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
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // Fixed Window Size. 
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

	// Blending
	//glEnable(GL_DEPTH_TEST); // 2D No depth in viewer app.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Render Operations
void Viewer::render()
{
	// ==================== Render State ====================
	glClearColor(0.1f, 0.1f, 0.1f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ==================== Render Fluid ====================
	if (ren_meta) fluid_object->render(Fluid_Object::Render_Type::METABALL, ortho);

	if (ren_pts)  fluid_object->render(Fluid_Object::Render_Type::POINT_VERTS, ortho);

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
	// Defered to GUI input
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
	static float jit = DEF_JIT;
	static float pos [2] = { DEF_XP, DEF_YP };
	static float dim [2] = { DEF_XS, DEF_YS };
	float pt_s = 1.f, surf_s = 1.f; 
	
	// Checkboxes (Local booleans)
	static bool compute_rest = true;
	static bool use_visc = false; 
	static bool use_surftens = false;

	// Get current pressure kernel string
	std::string kern_pres = ""; 
	if (fluid_solver->pressure_kernel == Fluid_Solver::kernel::POLY6) kern_pres = "Pressure Kernel : Poly6"; else kern_pres = "Pressure Kernel : Spiky";

	// Fluid Solver Core parameters (Reconstruct if changed)
	static float kernel_radius = 0.5f;

	// Get Dt 1/n. 
	float n = 1.f / fluid_solver->dt;
	static int tmp_count = 90;

	// =================== Imgui layout ===================
	{
		// Begin ImGui
		ImGui::Begin("Simulation Controls");

		// =================== Solver State ===================
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

		// =================== Fluid Attributes ===================
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("Fluid Attributes");
		ImGui::PopStyleColor();
		ImGui::Text("Particle Count : %d", fluid_object->particles.size());
		ImGui::Text("Simulation Domain : [0,10] (X,Y)");
		ImGui::Text(kern_pres.c_str());
		ImGui::Text("Mass : %f", fluid_object->particles[0].mass);
		ImGui::Text("Density :  min = %f | max = %f",  fluid_object->min_dens, fluid_object->max_dens);
		ImGui::Text("Pressure : min = %f | max = %f", fluid_object->min_pres, fluid_object->max_pres);
		ImGui::Text("ColorFld : min = %f | max = %f", fluid_object->min_cf,   fluid_object->max_cf);
		ImGui::Text("Grid : cell_size = %f | cell_count = %d", fluid_object->cell_s, fluid_object->cell_c);

		// =================== Fluid State Controls ===================
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("Fluid State Controls");
		ImGui::PopStyleColor();

		// Rest Density
		if (ImGui::Checkbox("Comput Rest Dens", &compute_rest))
		{
			fluid_solver->compute_rest = compute_rest;
		}
		ImGui::SliderFloat("Rest Dens", &fluid_solver->rest_density, 1.f, 1000.f);

		// =================== Rebuild Fluid Object / Solver if change ===================
		/* Info : as these parameters require rebuilding of the Fluid Object or solver, their state has to be transfered for persitence. 
		          a bit ugly looking within an imode gui. */ 
		if (ImGui::SliderFloat2("Fluid Pos", pos, 0.f, 10.f))
		{
			// Dealloc old fobj
			fluid_solver->simulate = false; 
			Fluid_Object::Colour_Viz o_c = fluid_object->particle_colour;
			float o_ps = fluid_object->pts_scale, o_ss = fluid_object->surf_scale;
			delete fluid_object;
			// Alloc New fobj
			fluid_object = new Fluid_Object(glm::vec2(pos[0], pos[1]), glm::vec2(dim[0], dim[1]), spc, jit);
			fluid_object->particle_colour = o_c;
			fluid_object->pts_scale = o_ps, fluid_object->surf_scale = o_ss;
			// Update Pipes 
			fluid_solver->fluidData = fluid_object;    // Pass FluidObj ref to FluidSolver
			fluid_object->solver    = fluid_solver;    // Pass FluidSolver ref to FluidObj
		}
		if (ImGui::SliderFloat2("Fluid Dim", dim, 0.f, 10.f))
		{
			// Dealloc old fobj
			fluid_solver->simulate = false;
			Fluid_Object::Colour_Viz o_c = fluid_object->particle_colour;
			float o_ps = fluid_object->pts_scale, o_ss = fluid_object->surf_scale;
			delete fluid_object;
			// Alloc New fobj
			fluid_object = new Fluid_Object(glm::vec2(pos[0], pos[1]), glm::vec2(dim[0], dim[1]), spc, jit);
			fluid_object->particle_colour = o_c;
			fluid_object->pts_scale = o_ps, fluid_object->surf_scale = o_ss;
			// Update Pipes 
			fluid_solver->fluidData = fluid_object;    // Pass FluidObj ref to FluidSolver
			fluid_object->solver    = fluid_solver;    // Pass FluidSolver ref to FluidObj

		}
		if (ImGui::SliderFloat("Fluid Spc", &spc, 1e-05f, 0.25f))
		{
			// Dealloc old fobj
			fluid_solver->simulate = false;
			Fluid_Object::Colour_Viz o_c = fluid_object->particle_colour;
			float o_ps = fluid_object->pts_scale, o_ss = fluid_object->surf_scale;
			
			delete fluid_object;
			// Alloc new fobj
			fluid_object = new Fluid_Object(glm::vec2(pos[0], pos[1]), glm::vec2(dim[0], dim[1]), spc, jit);
			fluid_object->particle_colour = o_c;
			fluid_object->pts_scale = o_ps, fluid_object->surf_scale = o_ss;
			// Update Pipes 
			fluid_solver->fluidData = fluid_object;     // Pass FluidObj ref to FluidSolver
			fluid_object->solver    = fluid_solver;     // Pass FluidSolver ref to FluidObj
		}
		if (ImGui::SliderFloat("Fluid Jitter", &jit, 0.f, 1.f))
		{
			// Dealloc old fobj
			fluid_solver->simulate = false;
			Fluid_Object::Colour_Viz o_c = fluid_object->particle_colour;
			float o_ps = fluid_object->pts_scale, o_ss = fluid_object->surf_scale;
			delete fluid_object;
			// Alloc new fobj
			fluid_object = new Fluid_Object(glm::vec2(pos[0], pos[1]), glm::vec2(dim[0], dim[1]), spc, jit);
			fluid_object->particle_colour = o_c;
			fluid_object->pts_scale = o_ps, fluid_object->surf_scale = o_ss;
			// Update Pipes 
			fluid_solver->fluidData = fluid_object;    // Pass FluidObj ref to FluidSolver
			fluid_object->solver    = fluid_solver;    // Pass FluidSolver ref to FluidObj
		}

		// Causes rebuild of Fluid_Solver if changed (this is so kernel coeffs can be pre-computed) 
		if (ImGui::SliderFloat("Kernel Radius", &kernel_radius, 0.1f, 2.f))
		{
			// Store old state and dealloc solver
			fluid_solver->simulate = false;
			float dt = fluid_solver->dt, rest_dens = fluid_solver->rest_density, stiff = fluid_solver->stiffness_coeff, g = fluid_solver->gravity, ar = fluid_solver->air_resist;
			bool b_visc = fluid_solver->use_visc, b_surf = fluid_solver->use_surftens;
			float f_visc = fluid_solver->k_viscosity, f_st = fluid_solver->k_surftens;
			delete fluid_solver;
			// Alloc new solver with updated kernel size and old state. 
			fluid_solver = new Fluid_Solver(dt, kernel_radius, fluid_object);
			fluid_solver->rest_density = rest_dens;
			fluid_solver->stiffness_coeff = stiff, fluid_solver->gravity = g, fluid_solver->air_resist = ar, fluid_solver->k_viscosity = f_visc, fluid_solver->k_surftens = f_st; 
			fluid_solver->use_visc = b_visc, fluid_solver->use_surftens = b_surf;
			fluid_object->solver = fluid_solver; // Pass FluidSolver ref to FluidObj
		}

		// =================== Pressure Kernel Selection ===================
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("Kernel Selection");
		ImGui::PopStyleColor();
		if (ImGui::Button("Poly6 Pressure"))
		{
			fluid_solver->pressure_kernel = Fluid_Solver::kernel::POLY6;
			std::cout << "Fluid Solver::Pressure Kernel = Poly6 Gradient\n";
		}
		if (ImGui::Button("Spiky Pressure"))
		{
			fluid_solver->pressure_kernel = Fluid_Solver::kernel::SPIKY;
			std::cout << "Fluid Solver::Pressure Kernel = Spiky Gradient\n";
		}

		// =================== Free parameters (min,max enforced here) ===================
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		// -------- Internal Forces 
		ImGui::Text("Internal Forces");
		ImGui::PopStyleColor();
		// Pressure Gas Constant / Stiffness 
		ImGui::SliderFloat("Pressure", &fluid_solver->stiffness_coeff, 0.f, 1000.f);
		// Viscosity
		if (ImGui::Checkbox("Use Viscosity", &use_visc))
		{
			fluid_solver->use_visc = !fluid_solver->use_visc;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		ImGui::SliderFloat("Viscosity", &fluid_solver->k_viscosity, 0.f, 100.f);
		// Surface Tension
		if (ImGui::Checkbox("Use Surface Tension", &use_surftens))
		{
			fluid_solver->use_surftens = !fluid_solver->use_surftens;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		ImGui::SliderFloat("Surface Tension", &fluid_solver->k_surftens, 0.f, 100.f);
		// -------- External Forces 
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("External Forces");
		ImGui::PopStyleColor();
		ImGui::SliderFloat("Gravity",   &fluid_solver->gravity,        -20.f, 10.f);
		ImGui::SliderFloat("AirResist", &fluid_solver->air_resist,       0.f, 5.f);

		// =================== Fluid Rendering ===================
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("Render Controls");
		ImGui::PopStyleColor();

		// Render Flags
		// Surface
		ImGui::Checkbox("Render Surface", &ren_meta);
		ImGui::SliderFloat("Surf Radius", &fluid_object->surf_scale, 0.1f, 5.f);
		ImGui::SliderFloat("Iso Threshold", &fluid_object->iso_thresh, 0.1f, 2.0f);
		// Particles
		ImGui::Checkbox("Render Particles",  &ren_pts);
		ImGui::SliderFloat("Pt Radius",      &fluid_object->pts_scale, 0.1f, 5.f);	
		// Particle Colours 
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 150, 255));
		ImGui::Text("Particle Mode");
		ImGui::PopStyleColor();
		if (ImGui::Button("Colour : Velocity"))
		{
			fluid_object->particle_colour = Fluid_Object::Colour_Viz::Velocity;
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
		if (ImGui::Button("Colour : ColField"))
		{
			fluid_object->particle_colour = Fluid_Object::Colour_Viz::Colour;
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

	// =================== Imgui Render ===================
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


