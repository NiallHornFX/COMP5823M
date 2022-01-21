// COMP5823M - A2 : Niall Horn - cloth_solver.cpp
// Implements
#include "cloth_solver.h"

// Std Headers
#include <iostream>


// ================================== Cloth_Solver Class Implementation ===============================

Cloth_Solver::Cloth_Solver(Cloth_State &ClothState, float Sim_Dt)
	: clothData(ClothState), dt(Sim_Dt), at(0.f)
{
	// Init 
	gravity = -1.0f;
	wind = glm::vec3(0.f);
	K_s = 1000.f, K_c = 10.f, K_v = 0.5f; 
	coeff_fric  = 1.f; 
	frame = 0, timestep = 0; 
	simulate = false; 
}

// Info : Tick Simulation for number of timesteps determined by viewer Dt. Uses Hybrid Timestepping approach purposed by Glenn Fiedler
// Physics Dt is constant (based on UI value), while using Viewer app Dt to define number of solve substeps.
void Cloth_Solver::tick(float viewer_Dt)
{
	if (!simulate || !clothData.built_state) return; 

	// Reset Timestep (solvestep) counter (per tick)
	timestep = 0; 

	// Subdivide Accumulated Viewer Timestep into Solver Substeps
	at += viewer_Dt;
	while (at > dt)
	{
		step(); // Single Timestep
		at -= dt;
		timestep++;
	}
	frame++;
}

// Info : Reset Cloth_State and Solver Time State
void Cloth_Solver::reset()
{
	// Reset Time state
	frame = 0, timestep = 0; 

	// Reset Cloth_State
	clothData.reset_cloth();
}


// Info : Single Simulation Step of Cloth Solver 
void Cloth_Solver::step()
{
	integrate_euler();

	eval_springs();

	eval_colliders();
}

// Info : Eval Springs and apply the resulting forces to their particles. 
void Cloth_Solver::eval_springs()
{
	// Clear prev particle forces. 
	for (Particle &p : clothData.particles) p.F.x = 0.f, p.F.y = 0.f, p.F.z = 0.f;

	for (Spring &s : clothData.springs)
	{
		// Fetch Deltas
		glm::vec3 p1p0   = s.pt_1->P - s.pt_0->P;
		glm::vec3 p1p0_n = glm::normalize(p1p0);
		glm::vec3 v1v0   = s.pt_1->V - s.pt_0->V;
		float length = glm::length(p1p0);

		// Spring : (-K_s * (||p1-p0||-rest) * (p1-p0 / ||p1-p0||)
		glm::vec3 f_spring = -K_s * (length - s.rest) * p1p0_n;
		// Damp   : (-K_c * ((v1-v0) dot (p1-p0/||p1-p0||)) * (p1-p0 / ||p1-p0||)
		glm::vec3 f_damper = -K_c * glm::dot(v1v0, p1p0_n) * p1p0_n;
		// Combined
		glm::vec3 sprdamp = f_spring + f_damper;

		// Apply equal opposite forces to particles of spring
		s.pt_0->F -= sprdamp, s.pt_1->F += sprdamp;
	}
}

// Info : Using Semi Implicit Forward Euler, Integrate forces to particle positions for (n+1)
void Cloth_Solver::integrate_euler()
{
	for (Particle &curPt : clothData.particles)
	{
		if (curPt.state == pt_state::FIXED) continue; // Skip Fixed Particles 

		// Particle Spring Forces + Wind
		glm::vec3 forces = curPt.F + wind;
		// Add Air Visocity Force 
		forces += -K_v * curPt.V; 

		// A = F/M (assuming mass != 1) 
		glm::vec3 accel = (forces / curPt.mass) + glm::vec3(0.f, gravity, 0.f);

		// ======= Integrate (Semi-Implicit Euler) =======
		// v_(n+1) = v(n) + a(n) * dt; 
		curPt.V += accel * dt;
		// x_(n+1) = x(n) + v(n+1) * dt; 
		curPt.P += curPt.V * dt;
	}
}


// Info : Evaulate Collisions using passed colliders.
void Cloth_Solver::eval_colliders()
{
	if (!colliders.size()) return; 

	for (Cloth_Collider *col : colliders)
	{
		if (!col) continue;
		col->eval_collision(clothData.particles);
	}
}

// Info : Set timestep from passed "steps per second" count. 
void Cloth_Solver::set_timestep(std::size_t count)
{
	if (count <= 10) return; 
	dt = 1.f / float(count);
}

// Info : Set eps for colliders.
void Cloth_Solver::set_collision_eps(float epsilon)
{
	for (Cloth_Collider *col : colliders)
	{
		if (col) col->collision_epsilon = epsilon;
	}
}

// Info : Set frictions for colliders.
void Cloth_Solver::set_collision_fric(float fric)
{
	for (Cloth_Collider *col : colliders)
	{
		if (col) col->friction = fric; 
	}
}