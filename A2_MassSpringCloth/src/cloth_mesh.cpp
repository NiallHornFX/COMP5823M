// COMP5823M - A2 : Niall Horn - cloth_mesh.cpp
// Implements
#include "cloth_mesh.h"

// Std Headers
#include <cmath>
#include <cassert>

// ================================== Cloth_Mesh Class Implementation ===============================

// Info : Construction of Cloth_Mesh, Pass Refereces to Cloth_State instances : particle and index data arrays. 
// Initalize Base Primtiive, Calc Inital Attributes of Particle-Verts and indices.
Cloth_Mesh::Cloth_Mesh(const std::vector<Particle> &array_particles, const std::vector<glm::ivec3> &array_tris, const ParticleTriList &array_ptTris)
	: Primitive("Cloth_Mesh"), particles(array_particles), tri_indices(array_tris), particle_tris(array_ptTris)
{
	// ========== Primitive Setup Cals ==========
	set_shader("../../shaders/cloth.vert", "../../shaders/cloth.frag");
	mode = Render_Mode::RENDER_MESH;
	ren_edges = false, ren_points = false; 

	// ========== Calc Attributes from Particles for Vert Data ==========
	// Calculate Normals Per Particle-Vert
	std::vector<glm::vec3> normals = calc_normals();
	// Get Inital UVs (assumes mesh is 2D Grid) 
	std::vector<glm::vec2> uv = calc_uvs();

	// ========== Serailize Particle Data to Vert Data array (Inital Data) ==========
	// Serailize Particle Attributes into float array in Attribute Order (P.xyz|N.xyz|C.rgb|UV.uv)
	std::vector<float> vert_data; 
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		// Position (x,y,z) 3f 12 bytes
		const Particle &curPt = particles[p];
		vert_data.push_back(curPt.P.x), vert_data.push_back(curPt.P.y), vert_data.push_back(curPt.P.z);
		// Normal (x,y,z) 3f 12 bytes
		const glm::vec3 &normal = normals[p];
		vert_data.push_back(normal.x), vert_data.push_back(normal.y), vert_data.push_back(normal.z);
		// Colour (r,g,b) 3f 12 bytes
		const glm::vec3 col(1.f, 0.f, 0.f);
		vert_data.push_back(col.r), vert_data.push_back(col.g), vert_data.push_back(col.b);
		// UV (u,v) 2f 8 bytes
		const glm::vec2 &t_uv = uv[p];
		vert_data.push_back(t_uv.x), vert_data.push_back(t_uv.y);
	}
	assert(vert_data.size() == (particles.size() * 11)); // Make sure 11 floats per particle.
	// Pass VertData to Primitive::set_data_mesh
	set_data_mesh(vert_data.data(), particles.size());

	// ========== Serailize Tri Indices to Index Array ==========
	// only done once here, cloth topo thus indices don't change.
	for (const glm::ivec3 &tri : tri_indices)
	{
		indices.push_back(tri[0]), indices.push_back(tri[1]), indices.push_back(tri[2]);
	}

	// ========== GL Buffer Setup / Alloc ==========
	// Buffer Setup (Primitive: VAO,VBO. Cloth_Mesh: EBO)
	create_buffers();
}

Cloth_Mesh::~Cloth_Mesh()
{
	if (EBO) glDeleteBuffers(1, &EBO);
}

// ==============================================================================================
//                              Primitive Virtual Overloads
// ==============================================================================================
// Info : Override Primitive::create_buffers() to setup Element Buffer, while still calling Primtiive::CreateBuffers for VAO,VBO.
void Cloth_Mesh::create_buffers()
{
	Primitive::create_buffers(); // VAO,VBO Setup

	// EBO Setup  
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sizeof(uint) * indices.size()), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Info : Override Primitive::render() to draw cloth mesh using indexed drawing. 
void Cloth_Mesh::render()
{
	// Check for state to render
	if (!check_state())
	{
		std::cerr << "ERROR::Cloth_Mesh::" << name << "::Render called, with incorrectly set state." << std::endl;
		std::terminate();
	}

	// Local Shader for rendering edges/wire + pts. 
	Shader wire_shader("cloth_wrie", "../../shaders/cloth.vert", "../../shaders/cloth_wire.frag");
	wire_shader.load();
	
	// Bind Buffers
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// ================ Render Mesh ================
	// Bind Primitive State
	shader.use();
	shader.setMat4("model", model);
	// Render Mesh
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

	// ================ Render Edges ================
	if (ren_edges)
	{
		// Bind Wire Shader State
		glUseProgram(0);
		wire_shader.use();
		wire_shader.setBool("wire", true);
		wire_shader.setMat4("model", model);
		wire_shader.setMat4("view", view);
		wire_shader.setMat4("proj", persp);

		// Render Edges
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	}
	// ================ Render Points ================
	if (ren_points)
	{
		// Use Wire Shader for pts
		wire_shader.setBool("wire", false);
		glPointSize(10.f);
		glDrawArrays(GL_POINTS, 0, vert_count);
	}

	// Clear State
	glUseProgram(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// ==============================================================================================
//                              Particle - Vertex Update Functions 
// ==============================================================================================
// Info :  Update per tick particle data via cloth_state particle array reference. (Oppose to using Primitive::update methods)
// Attrib Layout : Px,Py,Pz | Nx,Ny,Nz | Cr,Cg,Cb | U,V |  (11 x 4Byte fp32)
//                 0  1  2    3  4  5    6  7  8    9 10         
void Cloth_Mesh::update_fromParticles()
{
	if (!vert_data.size()) return; // Inital Cloth Vert Data must be set first. 

	// Get Updated Normals
	std::vector<glm::vec3> normals = calc_normals();

	// Update Particle-Vert Positions and Normals within Primitive::vert_data array. 
	// Attrib Layout (P,N,C,UV) C and UV are left unchanged. 
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		const Particle &curPt = particles[p];
		// Get Attrib start indices 
		// Vert Index, Position. (|a_P, a_P+1, a_P+2| (Nxyz)...)
		std::size_t a_P = p * 11;    
		// Vert Index, Normal. ((Pxyz) |a_N, a_N+1, a_N+2| (Crgb)...)
		std::size_t a_N = 3 + p * 11; 
		// Update Position Data
		vert_data[a_P++] = curPt.P.x, vert_data[a_P++] = curPt.P.y, vert_data[a_P] = curPt.P.z;
		// Update Normal Data 
		const glm::vec3 &N = normals[p];
		vert_data[a_N++] = N.x, vert_data[a_N++] = N.y, vert_data[a_N] = N.z;
	}

	// Refill Buffer (not ideal)
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (vert_count * 11 * sizeof(float)), vert_data.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ==============================================================================================
//                              Attribute Setup Functions
// ==============================================================================================

// Info : Better normal calculation approach, calc normals per tri, then per particle average its triangles normals. 
std::vector<glm::vec3> Cloth_Mesh::calc_normals()
{
	// First Calc Normals Per Tri
	std::vector<glm::vec3> tri_normals(tri_indices.size());
	for (std::size_t t = 0; t < tri_indices.size(); ++t)
	{
		glm::vec3 v0 = particles[tri_indices[t][0]].P;
		glm::vec3 v1 = particles[tri_indices[t][1]].P;
		glm::vec3 v2 = particles[tri_indices[t][2]].P;

		glm::vec3 tang   = glm::normalize(v1 - v0);
		glm::vec3 bitang = glm::normalize(v2 - v0);
		glm::vec3 normal = glm::normalize(glm::cross(tang, bitang));
		
		tri_normals[t] = std::move(normal);
	}

	// Per Particle Average Normals of faces its part of.
	std::vector<glm::vec3> normals(particles.size());
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		const Particle &curPt = particles[p];
		glm::vec3 normal(0.f);

		// Get Per Particle Triangles, Accumulate Normals of each tri. 
		std::vector<glm::ivec4> ptTris = particle_tris[p];
		for (std::size_t t = 0; t < ptTris.size(); ++t)
		{
			int tri_idx = particle_tris[p][t].w;
			normal += tri_normals[tri_idx];
		}
		// Average resulting Normal by PtTri Count
		normal /= float(ptTris.size());
		normals[p] = std::move(normal);

		// Debug
		//std::cout << "particle_" << p << " ID = " << curPt.id << " Normal = " << normal.x << "," << normal.y << "," << normal.z << "\n";
	}
	return normals; 
}

// Assumes mesh is a uniform 2D Grid, uses 2D indexing to calculate UVs as such. Won't work for any other mesh ofcourse. 
std::vector<glm::vec2> Cloth_Mesh::calc_uvs()
{
	// Defines single dimension size of grid. 
	std::size_t m = static_cast<std::size_t>(std::sqrt(particles.size()));
	float m_r = 1.f / float(m-1);

	auto idx_1dto2D = [](std::size_t i, std::size_t m) -> glm::ivec2
	{
		return glm::ivec2((i / m), (i % m));
	};

	// Approximate UV Coords over 2D grid.  
	std::vector<glm::vec2> pt_uv(particles.size());
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		const Particle &curPt = particles[p];
		glm::ivec2 ij = idx_1dto2D(p, m);

		float u = float(ij[0]) * m_r; 
		float v = float(ij[1]) * m_r; 
		pt_uv[p] = std::move(glm::vec2(u, v));

		// Debug
		//std::cout << "particle_" << p << " UV = " << u << "," << v << "\n";
	}
	return pt_uv;
}


