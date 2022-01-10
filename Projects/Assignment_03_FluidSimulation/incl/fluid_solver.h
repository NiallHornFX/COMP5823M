// COMP5823M - A3 : Niall Horn - fluid_solver.h
#ifndef FLUID_SOLVER_H
#define FLUID_SOLVER_H

// Std Headers
#include <vector>

// Project Headers

// Ext Headers 
// GLM
#include "ext/glm/glm.hpp"

class Fluid_Object; 
class Fluid_Collider; 
class Hash_Grid;
class Spatial_Grid;

struct Particle; 

#define INLINE __forceinline 

class Fluid_Solver
{
public: 
	Fluid_Solver(float Sim_Dt, float RestDens, float KernelRad, Fluid_Object *Data);
	~Fluid_Solver() = default; 

	// Kernel Function Pointers
	using kernel_func      = float(Fluid_Solver::*) (const glm::vec3 &r);
	using kernel_grad_func = glm::vec2(Fluid_Solver::*)(const glm::vec3 &r);
	using kernel_lapl_func = float(Fluid_Solver::*) (const glm::vec3 &r);

	// ======= Operations =======
	void reset();

	void tick(float viewer_Dt);

	void step();

	void render_colliders(const glm::mat4 &ortho);

	// ======= Solver =======
	void get_neighbours();

	void eval_colliders();

	void compute_dens_pres(kernel_func w);

	glm::vec3 eval_forces(Particle &Pt_i, kernel_func w, kernel_grad_func w_g);

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
	Hash_Grid *hg;
	Spatial_Grid *accel_grid; 
	bool got_neighbours; 

	// ======= Forces =======
	// Ext Forces
	float gravity, air_resist; 
	// Internal Force Coeffs 
	float k_viscosity, k_surftens; 
	
	// ======= SPH =======
	float kernel_radius, kernel_radius_sqr; // h

	// Precomputed Kernel + Derivative Scalar Coeffecients 
	float poly6_s, spiky_s; 
	float poly6_grad_s, spiky_grad_s; 
	float visc_lapl_s; 

	// Pressure
	float rest_density, stiffness_coeff;

	// Min/Max Attrib Ranges
	float min_dens,  max_dens;
	float min_pres,  max_pres; 
	float min_force, max_force; 

	// Fluid Colliders
	std::vector<Fluid_Collider*> colliders;

	// Fluid State Object
	Fluid_Object *fluidData; 
};


#endif