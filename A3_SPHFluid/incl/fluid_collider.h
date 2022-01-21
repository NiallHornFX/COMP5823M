// COMP5823M - A3 : Niall Horn - fluid_collider.h
#ifndef FLUID_COLLIDER_H
#define FLUID_COLLIDER_H

// Std Headers
#include <vector>
#include <string>

// Project Headers
#include "primitive.h"

// Ext Headers 
#include "ext/glm/glm.hpp" // GLM

struct Particle; 

// Info : A basic polymorphic collider class to implement primtivie collision detection and response 
//        with passed fluid particles. 

// Base Collider Abstract Base 
class Fluid_Collider
{
public:
	Fluid_Collider(const char *Name);
	virtual ~Fluid_Collider() = default;

	virtual void eval_collision(std::vector<Particle> &particles) = 0;

public:
	std::string name;
	Primitive *prim;

	float eps, friction;
	bool render;
};

// Plane Collider 
class Fluid_Collider_Plane : public Fluid_Collider
{
private:
	enum Type {HORIZONTAL = 0, VERTICAL};

public:
	Fluid_Collider_Plane(const char *name, const glm::vec3 &Q, const glm::vec3 &Normal, const glm::vec2 &LH);

	virtual void eval_collision(std::vector<Particle> &particles) override final;

private:
	void render_setup();

	glm::vec3 q, N; // q defines start position, not center. 
	float length, height; 
	Type type; 
};



#endif