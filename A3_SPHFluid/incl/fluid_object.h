// COMP5823M - A3 : Niall Horn - fluid_object.h
#ifndef FLUID_OBJECT_H
#define FLUID_OBJECT_H

// Std Headers
#include <vector>
#include <random>

// Project Headers
#include "primitive.h"

// Ext Headers 
#include "ext/glm/glm.hpp" // GLM

#define INLINE __forceinline 

// Defaults
#define DEF_XP 2.75f
#define DEF_YP 4.f
#define DEF_XS 2.f
#define DEF_YS 3.f
#define DEF_SPC 0.2f
#define DEF_JIT 0.0f

// FD
struct Particle; 
class Fluid_Solver; 

// =================================== Fluid Object Class ===================================
// Info : Class containing fluid state and data, also responsible for rendering the fluid using
//        metaball or point based render paths. 

class Fluid_Object
{
public:
	Fluid_Object(const glm::vec2 &P = glm::vec2(DEF_XP, DEF_YP), const glm::vec2 &Dim = glm::vec2(DEF_XS, DEF_YS), float Spc = DEF_SPC, float Jitter = DEF_JIT);
	~Fluid_Object(); 

	void emit_square();
	void reset_fluid();

	// ======== Render ========
	enum Render_Type {POINT_VERTS = 0, METABALL};
	void render_setup();
	void render(Render_Type mode, const glm::mat4 &ortho);


public: 
	// ======== Fluid Data ========
	std::vector<Particle> particles; 
	std::vector<std::vector<Particle*>> particle_neighbours; 
	glm::vec2 pos, dim; 
	float spc, jitter; 

	// Min/Max Attrib Ranges
	float min_dens,  max_dens;
	float min_cf,    max_cf; 
	float min_pres,  max_pres;
	float max_spd; 

	// Accel Grid State (for viewer)
	float cell_s; 
	std::size_t cell_c; 

	// ======== Render Primitives ========
	Primitive *ren_points; // Point Rendering via Primitive

	// Rendering grid within Fragment Shader
	Primitive *ren_quad; 

	// Render State 
	enum Colour_Viz { Velocity = 0, Density, Pressure, Colour, GridCell };
	Colour_Viz particle_colour;
	float pts_scale, surf_scale; 
	float iso_thresh; 

	// OpenGL Data
	GLuint ssbo_pts; 

	Fluid_Solver *solver; // Fluid Solver ref (for bidir pipe)
};


// =================================== Particle Struct ===================================
struct Particle
{
	Particle(const glm::vec3 &p, float Mass, std::size_t idx)
		: P(p), mass(Mass), rest(P), V(glm::vec3(0.f)), id(idx), cell_idx(-1),
		   cf(0.f), density(0.f), pressure(0.f) {}

	glm::vec3 P, V;
	glm::vec3 rest; 
	std::size_t id, cell_idx;
	float density, pressure, mass, cf;
};

struct alignas(16) Particle_GPU
{
	glm::vec2 pos;
	glm::vec2 vel; 
	float dens; 
};

// =================================== Util Functions ===================================
INLINE float fitRange(float val, float a_min, float a_max, float b_min, float b_max)
{
	return b_min + (val - a_min)*(b_max - b_min) / (a_max - a_min);
}

INLINE glm::vec3 lerp_vec(glm::vec3 a, glm::vec3 b, float t)
{
	float x = a.x * (1.f - t) + b.x * t; 
	float y = a.y * (1.f - t) + b.y * t;
	float z = a.z * (1.f - t) + b.z * t;
	return glm::vec3(x, y, z);
}

INLINE glm::vec3 randRange(std::size_t seed, float min, float max)
{
	std::mt19937_64 rng;
	std::uniform_real_distribution<float> dist(min, max);
	rng.seed(seed);        float x = dist(rng);
	rng.seed(seed + 124);  float y = dist(rng);
	rng.seed(seed + 321);  float z = dist(rng);
	return glm::vec3(x, y, z);
}

#endif