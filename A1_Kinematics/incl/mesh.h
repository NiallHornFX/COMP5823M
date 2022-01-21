#ifndef MESH_H
#define MESH_H

// Std Headers
#include <string>
#include <vector>

// Ext Headers 
// GLM
#include "ext/glm/glm.hpp"

// Project Headers
#include "primitive.h"
#include "texture.h"

// Info : Primitive based class for rendering meshes based on loaded .obj file, along with textures. 
// Vertex Attribute layout same as Primitive (P(x,y,z), N(x,y,z), C(r,g,b), UV(u,v))

class Mesh : public Primitive
{
public:
	Mesh() : Primitive("") {}
	Mesh(const char *name, const char *filePath);
	~Mesh();

	virtual void render()            override; 
	virtual bool check_state() const override;

	void load_obj(bool has_tex = false);
	void load_texture(const char *filepath, uint unit = 0);

public:
	std::string file_path; 
	struct
	{
		std::vector<glm::vec3> v_p;
		std::vector<glm::vec3> v_n;
		std::vector<glm::vec2> v_t; 
		std::vector<vert> verts;
	} obj_data;

	// State
	bool has_tex;
	bool wireframe; 

	// Texture 
	bool use_tex;
	Texture *tex; 
};


#endif