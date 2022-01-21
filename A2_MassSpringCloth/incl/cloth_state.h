// COMP5823M A2 : Niall Horn 201486968 - cloth_state.h
#ifndef CLOTH_STATE_H
#define CLOTH_STATE_H

// Std Headers
#include <vector>
#include <string>
#include <fstream>

// Project Headers
#include "cloth_common.h"
#include "cloth_mesh.h"

// Ext Headers
#include "ext/glm/glm.hpp"

// FDs
class Primitive; 

class Cloth_State
{
public:
	Cloth_State(const char *path); 
	~Cloth_State();

	// ======= Operations =======
	void load_obj(std::ifstream &in);

	void build_cloth_springs(); 

	void reset_cloth();

	void render(const glm::mat4x4 &view, const glm::mat4x4 &persp);

	void export_mesh(const char *export_path);

	// ======= Util =======
	void get_particle_trilist(); 

	void set_fixed_corners(bool state); 

	void set_rest_offset(const glm::vec3 &offset);

private:

	// Input Mesh Data
	std::string file_path; 
	std::vector<glm::vec3>  v_p;
	std::vector<glm::ivec3> tris; 

	// Cloth Sim Data
	std::vector<Particle> particles; 
	std::vector<Spring>   springs; 
	ParticleTriList       pt_tris; // Per Particle Array of triangles.

	// Cloth Render Data
	Cloth_Mesh *mesh; 

	// Visualizer Data
	std::vector<Primitive*> viz_springs; 
	std::vector<Primitive*> viz_pts; 

	bool built_state; 
	glm::vec3 rest_offset;

	friend class Cloth_Solver; 
	friend class Cloth_Mesh;
	friend class Viewer; 
};



#endif