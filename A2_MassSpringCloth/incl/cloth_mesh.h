// COMP5823M - A2 : Niall Horn - cloth_mesh.h
#ifndef CLOTH_MESH_H
#define CLOTH_MESH_H

// Project Headers
#include "primitive.h"
#include "cloth_common.h"

// Info : Class for drawing cloth mesh from particle data, derived from Primtive class.
// This was gonna be a custom class, but will see how this goes first. 
// Unlike Primitive it uses indexed drawing. 

class Cloth_Mesh : public Primitive
{
public:
	// Setup inital atrrib and buffer state from passed pts. 
	Cloth_Mesh(const std::vector<Particle> &array_particles, const std::vector<glm::ivec3> &array_tris, const ParticleTriList &array_ptTris);
	Cloth_Mesh() = delete;
	virtual ~Cloth_Mesh() override; 

	virtual void render()         override; 

	virtual void create_buffers() override; 

	// Update / Attrib Calc Functions 
	void update_fromParticles();

	std::vector<glm::vec3> calc_normals();

	std::vector<glm::vec2> calc_uvs();

public:
	// Render Options 
	bool ren_edges;
	bool ren_points;

	// Set Cam Matrices for additonal shaders (other than Primtivie::shader)
	glm::mat4 view, persp;

private:
	// Serailzied Data
	std::vector<uint> indices; 
	// vertices are within : Primitive::vert_data

	// Reference Cloth_State Data Arrays : Particles, Indices and per particle tris arrays. 
	const std::vector<Particle> &particles;
	const std::vector<glm::ivec3> &tri_indices; 
	const ParticleTriList &particle_tris; 
	// GL Data
	GLuint EBO; 
	std::size_t indices_count;
};




#endif