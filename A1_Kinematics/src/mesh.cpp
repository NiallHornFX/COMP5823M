// Implements
#include "mesh.h"

// Std Headers
#include <fstream>
#include <sstream>

// #define DEBUG_LOG


Mesh::Mesh(const char *name, const char *filePath)
	: Primitive(name), file_path(filePath)
{
	use_tex = false; 
	tex = nullptr;
	has_tex = false;
	wireframe = false; 
	mode = Render_Mode::RENDER_MESH;
}

Mesh::~Mesh()
{
	if (tex) delete tex; 
}

// For now mesh is treated as triangle soup. No reuse of shared vertices. 
// Very basic, assumes obj file has tris. 
void Mesh::load_obj(bool has_tex)
{
	// Check file exists
	std::ifstream in(file_path);
	if (!in.is_open())
	{
		std::cerr << "ERROR::Mesh::" << name << ":: Invalid .obj file passed" << std::endl;
		return;
	}

	// Debug Stream
	std::ostringstream dbg; 

	std::string line;
	while (std::getline(in, line))
	{
		// Extract first block as str. 
		std::string str;
		std::istringstream ss(line);
		ss >> str;

		dbg << "DEBUG " << str << "\n";

		if (str == "#" || str == "g" || str == "s") continue;

		// Vertex Postion
		if (str == "v")
		{
			float xx, yy, zz;
			ss >> xx;
			ss >> yy;
			ss >> zz;
			obj_data.v_p.emplace_back(xx, yy, zz);
			dbg << "v_" << obj_data.v_p.size() << " = " << xx << "," << yy << "," << zz << "\n";
		}
		// Vertex Normal 
		if (str == "vn")
		{
			float xx, yy, zz;
			ss >> xx;
			ss >> yy;
			ss >> zz;
			obj_data.v_n.emplace_back(xx, yy, zz);
			dbg << "vn_" << obj_data.v_p.size() << " = " << xx << "," << yy << "," << zz << "\n";
		}
		// Vertex Texture
		if (has_tex)
		{
			// Vertex Texture Coord 
			if (str == "vt")
			{
				float uu, vv; 
				ss >> uu;
				ss >> vv;
				obj_data.v_t.emplace_back(uu, vv);
				dbg << "vt_" << obj_data.v_p.size() << " = " << uu << "," << vv << "\n";
			}
		}
		// Faces / Indices
		if (str == "f")
		{
			if (has_tex) // Get each face vertex (w/ texture coords)
			{
				for (int i = 0; i < 3; ++i)
				{
					// Vert Postion & Normal Indices. 
					int32_t i_vp, i_vn, i_vt;
					char c; // scratch write

					// Face Vertex Data
					ss >> i_vp; // v_p index
					ss >> c; // '/'
					ss >> c; // '/'
					ss >> i_vt; // 'v_t' index
					ss >> c; // '/'
					ss >> c; // '/'
					ss >> i_vn; // 'v_n' index

					// Create Vertex
					vert vertex;
					// neg 1 offset for obj indices '1' based. 
					vertex.pos    = obj_data.v_p[i_vp - 1];
					vertex.normal = obj_data.v_n[i_vn - 1];
					vertex.uv     = obj_data.v_t[i_vt - 1];
					vertex.col    = glm::vec3(1.f, 1.f, 1.f);

					// Append to vert array
					obj_data.verts.push_back(std::move(vertex));
				}
			}
			else // Get each face vertex (wo/ texture coords)
			{
				for (int i = 0; i < 3; ++i)
				{
					// Vert Postion & Normal Indices. 
					int32_t i_vp, i_vn;
					char c; // scratch write

					// Face Vertex Data
					ss >> i_vp; // v_p index
					ss >> c; // '/'
					ss >> c; // '/'
					ss >> i_vn; // 'v_n' index

					// Create Vertex
					vert vertex;
					// neg 1 offset for obj indices '1' based. 
					vertex.pos    = obj_data.v_p[i_vp - 1];
					vertex.normal = obj_data.v_n[i_vn - 1];
					vertex.uv     = glm::vec2(0.f, 0.f);
					vertex.col    = glm::vec3(1.f, 1.f, 1.f);

					// Append to vert array
					obj_data.verts.push_back(std::move(vertex));
				}
			}
		}

	} // End file read loop. 
	// Close 
	in.close();

	dbg << "Vert Count = " << obj_data.verts.size() << "\n";

	// Pass verts to primitive::mesh_data
	set_data_mesh(obj_data.verts);

	// Check Primitive is correct.
	//debug();

#ifdef DEBUG_LOG
		std::cout << dbg.str();
#endif
}

void Mesh::load_texture(const char *filepath, uint unit)
{
	std::string tex_name = name + " texture";
	tex = new Texture(tex_name.c_str(), filepath, unit);
	tex->load();

	if (tex->valid_state) has_tex = true; else return; 
	shader.setInt("tex", unit);
	use_tex = true; 
}

// Add Texture requirement to check_state if used. 
bool Mesh::check_state() const
{
	if (use_tex)
	{
		return Primitive::check_state() & has_tex;
	}
	return Primitive::check_state();
}

// Override Primitive::Render()
void Mesh::render()
{
	// Check for state to render
	if (!check_state())
	{
		std::cerr << "ERROR::Mesh::" << name << "::Render called, with incorrectly set state." << std::endl;
		std::terminate();
	}

	// Bind Primitive State
	shader.use();

	// Update Modfied Uniforms
	shader.setMat4("model", model);

	// Activate and Bind Texture
	if (use_tex)
	{
		tex->activate();
		tex->bind();
	}

	// Draw 
	glBindVertexArray(VAO);
	switch (mode)
	{
		case (RENDER_POINTS):
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			glDrawArrays(GL_TRIANGLES, 0, vert_count);
			break;
		}
		case (RENDER_LINES):
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_TRIANGLES, 0, vert_count);
			break;
		}
		case (RENDER_MESH):
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawArrays(GL_TRIANGLES, 0, vert_count);
			break;
		}
	}

	// Clear State
	glUseProgram(0);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}