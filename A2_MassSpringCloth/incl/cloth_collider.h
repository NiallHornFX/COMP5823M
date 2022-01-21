// COMP5823M - A2 : Niall Horn - cloth_collider.h
#ifndef CLOTH_COLLIDER_H
#define CLOTH_COLLIDER_H

// Std Headers
#include <vector>
#include <string>

// Project Headers
#include "cloth_common.h"
#include "mesh.h"

// Ext Headers 
#include "ext/glm/glm.hpp" // GLM

// Basic classes for evaluating Collision Primitives with Particles Array and rendering them. 

// Base Collider Abstract Base 
class Cloth_Collider
{
public:	
	Cloth_Collider(const char *Name, const char *renderMesh_path);
	virtual ~Cloth_Collider(); 

	virtual void eval_collision(std::vector<Particle> &particles) = 0; 

public:
	float collision_epsilon;
	float friction;
	std::string name; 
	Mesh *render_mesh; 
	bool render; 
};

// Plane Collider 
class Cloth_Collider_Plane : public Cloth_Collider
{
public:
	Cloth_Collider_Plane(const glm::vec3 &Normal);

	virtual void eval_collision(std::vector<Particle> &particles) override final;

private:
	glm::vec3 N; 
};

// Sphere Collider
class Cloth_Collider_Sphere : public Cloth_Collider
{
public:
	Cloth_Collider_Sphere(const glm::vec3 &Cent, float Radius);

	virtual void eval_collision(std::vector<Particle> &particles) override final; 

	// Setters (with render_mesh update) 
	void set_radius(float Rad);

	void set_centre(const glm::vec3 &cent);

private:
	glm::vec3 centre; 
	float radius; 

};


#endif