// COMP5823M - A2 : Niall Horn - cloth_solver.h
#ifndef CLOTH_SOLVER_H
#define CLOTH_SOLVER_H

// Std Headers

// Ext Headers
#include "ext/glm/glm.hpp"

// Project Headers
#include "cloth_state.h"
#include "cloth_collider.h"


class Cloth_Solver
{
public:
	Cloth_Solver(Cloth_State &ClothState, float Sim_Dt); 
	~Cloth_Solver() = default; 

	// ======= Operations =======
	void reset(); 

	void tick(float viewer_Dt);

	// ======= Solver =======
	void step();

	void eval_springs();

	void integrate_euler();

	void eval_colliders();

	// ======= Util =======
	void set_timestep(std::size_t count);
	void set_collision_eps(float epsilon);
	void set_collision_fric(float fric);

public:
	// Solver Intrinsics
	bool simulate; 
	float at, dt; // Acumulated Time, Fixed Physics timestep. 
	std::size_t frame, timestep;

	// Forces
	glm::vec3 wind; 
	float gravity; 

	// Cloth Colliders
	std::vector<Cloth_Collider*> colliders; 

	// Stiffness, Damping, Air Viscosity Coeffs. 
	float K_s, K_c, K_v; 
	// Collision Friction Coeff
	float coeff_fric; 

private:

	// Cloth_State Ref
	Cloth_State &clothData;

};

#endif
