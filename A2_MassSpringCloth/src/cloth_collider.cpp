// COMP5823M - A2 : Niall Horn - cloth_collider.cpp
// Implements
#include "cloth_collider.h"

// ================================== Cloth_Collider Class Implementation ===============================
Cloth_Collider::Cloth_Collider(const char *Name, const char *renderMesh_path)
	: name(Name), collision_epsilon(1e-02f), friction(0.25f)
{
	if (renderMesh_path)
	{
		render_mesh = new Mesh(Name, renderMesh_path);
		render_mesh->load_obj(false);
		render_mesh->set_shader("../../shaders/collider.vert", "../../shaders/collider.frag");
		render_mesh->set_colour(glm::vec3(1.f, 0.f, 0.f));
	}
	else render = false;
}

Cloth_Collider::~Cloth_Collider()
{
	if (render_mesh) delete render_mesh;
}

// ================================== Cloth_Collider_Plane Implementation ===============================
// Info : Plane has no mesh to draw (as we use ground_plane / grid in veiwer app) 
Cloth_Collider_Plane::Cloth_Collider_Plane(const glm::vec3 &Normal)
	: Cloth_Collider("Collision_Plane", nullptr), N(Normal) {}

// Info : Project positions of particles directly to satisfy the Plane collision inequality condition : 
// (P-Q) * N >= 0 (omit Q as we assume plane is at (0,0,0))
void Cloth_Collider_Plane::eval_collision(std::vector<Particle> &particles)
{
	for (Particle &curPt : particles)
	{
		float dist = glm::dot(curPt.P, N);
		if (dist <= 1e-03f) curPt.P += -dist * N;
	}
}

// ================================== Cloth_Collider_Sphere Implementation ===============================

Cloth_Collider_Sphere::Cloth_Collider_Sphere(const glm::vec3 &Cent, float Radius)
	: Cloth_Collider("Collision_Sphere", "../../assets/mesh/sphereB.obj"), centre(Cent), radius(Radius) 
{
	// Set Model Transform 
	render_mesh->scale(glm::vec3(radius));
	render_mesh->translate(centre);
}

// Info : Project positions of particles directly to satisfy the Sphere collision inequality condition : 
// ||P-C|| - r >= 0 (+ eps), for now we assume particles have zero radii. 
void Cloth_Collider_Sphere::eval_collision(std::vector<Particle> &particles)
{
	for (Particle &curPt : particles)
	{
		glm::vec3 vec = curPt.P - centre;
		float dist = glm::length(vec);
		if (dist < (radius + collision_epsilon))
		{
			float inter_dist = (radius + collision_epsilon) - dist;
			curPt.P += inter_dist * glm::normalize(vec);
			//curPt.F += -curPt.V * inter_dist * 100.f;

			// Decompose TangNorm for Friction (via Velocity)
			glm::vec3 N = glm::normalize(vec);
			glm::vec3 v_N = glm::dot(curPt.V, N) * N;
			glm::vec3 v_T = curPt.V - v_N;
			// Input fric (0-1) 
			float fric = friction * 0.1f; 
			// Effective range v_T * (0.9-1.0)
			curPt.V = (v_N + (v_T * (1.f - fric)));
		}
	}
}

// Reset Radius with new Radius
void Cloth_Collider_Sphere::set_radius(float Rad)
{
	radius = Rad; 
	render_mesh->model = glm::mat4(1.f);
	render_mesh->scale(glm::vec3(radius));
	render_mesh->translate(centre);
}

// Reset Centre with new Centre
void Cloth_Collider_Sphere::set_centre(const glm::vec3 &cent)
{
	render_mesh->model = glm::mat4(1.f);
	render_mesh->scale(glm::vec3(radius));
	render_mesh->translate(cent);
	centre = cent; 
}