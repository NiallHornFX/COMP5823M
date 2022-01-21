// COMP5823M - A3 : Niall Horn - fluid_solver.h
#ifndef FLUID_SOLVER_H
#define FLUID_SOLVER_H

// Std Headers
#include <vector>

// Ext Headers 
// GLM
#include "ext/glm/glm.hpp"

// Forward Decls 
class Fluid_Object; 
class Fluid_Collider; 
class Spatial_Grid;
struct Particle; 

#define INLINE __forceinline 

class Fluid_Solver
{
public: 
	Fluid_Solver(float Sim_Dt, float KernelRad, Fluid_Object *Data);
	~Fluid_Solver() = default; 

	// Kernel Function Pointers
	using kernel_func      = float(Fluid_Solver::*) (const glm::vec3 &r);
	using kernel_grad_func = glm::vec2(Fluid_Solver::*)(const glm::vec3 &r);
	using kernel_lapl_func = float(Fluid_Solver::*) (const glm::vec3 &r);

	enum kernel { POLY6 = 0, SPIKY, VISC };

	// ======= Operations =======
	void reset();

	void tick(float viewer_Dt);

	void step();

	void render_colliders(const glm::mat4 &ortho);

	void calc_colour_field();

	// ======= Solver =======
	void get_neighbours();

	void eval_colliders();

	void compute_dens_pres(kernel_func w);

	glm::vec3 eval_forces(Particle &Pt_i, kernel_grad_func w_pres_grad, kernel_grad_func w_surf_grad, kernel_lapl_func w_visc_lapl);

	void integrate();

	// ======= Util =======
	void calc_restdens();

	// ======= Smoothing Kernel Functions =======
	INLINE float kernel_poly6(const glm::vec3 &r);
	INLINE float kernel_spiky(const glm::vec3 &r);

	INLINE glm::vec2 kernel_poly6_gradient(const glm::vec3 &r);
	INLINE glm::vec2 kernel_spiky_gradient(const glm::vec3 &r);

	INLINE float kernel_visc_laplacian(const glm::vec3 &r);

public:
	// ======= Solver Intrinsics =======
	bool simulate;
	float at, dt; // Acumulated Time, Fixed Physics timestep. 
	std::size_t frame, timestep;
	Spatial_Grid *accel_grid; 
	bool got_neighbours; 

	bool compute_rest;

	// Kernel Selection
	kernel pressure_kernel, surftens_kernel;

	// ======= Forces =======
	// Ext Forces
	float gravity, air_resist; 
	// Internal Force Coeffs 
	float k_viscosity, k_surftens; 
	bool use_visc, use_surftens; 
	
	// ======= SPH =======
	float kernel_radius, kernel_radius_sqr; // h

	// Precomputed Kernel + Derivative Scalar Coeffecients 
	float poly6_s,      spiky_s; 
	float poly6_grad_s, spiky_grad_s; 
	float visc_lapl_s; 

	// Pressure
	float rest_density, stiffness_coeff;

	// Fluid Colliders
	std::vector<Fluid_Collider*> colliders;

	// Fluid State Object
	Fluid_Object *fluidData; 
};


#endif