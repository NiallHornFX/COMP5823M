// Implements
#include "cloth_state.h"

// Std Headers
#include <iostream>
#include <sstream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>

// Debug Defines
//#define DEBUG_LOG_OBJMESH
//#define DEBUG_LOG_PTTRIS
//#define DEBUG_LOG_SPRINGS

// =================================== Cloth_State Implementation ===================================

Cloth_State::Cloth_State(const char *path)
	: file_path(path), built_state(false), rest_offset(0.f)
{
	// Check OBJ Filepath
	std::ifstream obj_file(path);
	if (!obj_file.is_open())
	{
		std::cerr << "ERROR::File Path Not Valid : " << path << " !\n";
		std::terminate();
	}

	// Load OBJ Mesh
	load_obj(obj_file);

	// Set Rest Offset
	set_rest_offset(glm::vec3(0.f, 2.f, 0.f));

	// Pin Corners (test)
	set_fixed_corners(true);

	// Build Cloth State
	build_cloth_springs();

	// Create Cloth_Mesh (Pass it references to our data arrays)
	mesh = new Cloth_Mesh(particles, tris, pt_tris);

	built_state = true; 
}

Cloth_State::~Cloth_State()
{
	delete mesh; 
}

// Info : Parsing of .obj to tri-soup based verts.
// We discard vertex attributes so we can define unqiue vertices (and thus particles/point masses) based on postions.
// We will re-calc the normals anyway and UV Coords can be approximated later. 
void Cloth_State::load_obj(std::ifstream &in)
{
	std::stringstream dbg;
	dbg << "========== OBJ LOAD DEBUG BEGIN ==========\n";
	std::string line;
	bool has_n, has_t; 
	while (std::getline(in, line))
	{
		// Extract first block as str.
		std::string str;
		std::istringstream ss(line);
		ss >> str;

		// Skip these lines
		if (str == "#" || str == "g" || str == "s") continue;

		// Vertex Postion
		if (str == "v")
		{
			float xx, yy, zz;
			ss >> xx;
			ss >> yy;
			ss >> zz;
			dbg << "Vert-Pt_" << v_p.size() << " = " << xx << "," << yy << "," << zz << "\n";

			// Create Particle for unique vert position
			particles.emplace_back(glm::vec3(xx, yy, zz), particles.size());

			// And store as Rest Position (Per vert / Particle)
			v_p.emplace_back(xx, yy, zz);
			continue;
		}
		// Info : We will re-calculate other vertex attributes later, so discard these for now.
		//		  Store if they are present. 
		// Vertex Texture
		if (str == "vt")
		{
			has_t |= true; 
			continue;
		}
		// Vertex Normal
		if (str == "vn")
		{
			has_n |= true; 
			continue;
		}
		// Store Tri Indices per vert --> particle. 
		if (str == "f")
		{
			glm::ivec3 tri; // Hold Triangle Indices
			for (int i = 0; i < 3; ++i) // Per Tri Vert-->Pt
			{
				// Vert Postion Index
				int32_t i_vp;

				// Face Vertex Data
				ss >> i_vp;          // 'v_p' index
				ss.get(); ss.get();  // '//'  sep
				ss.get();            // 'v_n' index (discard) 

				// Create Particle (Each Unique Vert Position)
				std::size_t idx = i_vp - 1;
				tri[i] = idx; // Tri Ind
			}
			dbg << "Tri_" << tris.size() << " :: " << tri[0] << "," << tri[1] << "," << tri[2] << "\n";
			// Append Triangle
			tris.push_back(std::move(tri));
		}
	} 

	// Close file
	in.close();

	// Debug Output
	dbg << "Indexed Vert / Particle Count = " << particles.size() << "\n";
	dbg << "========== OBJ LOAD DEBUG END ==========\n";

#ifdef DEBUG_LOG_OBJMESH
	std::cout << dbg.str() << std::endl;
#endif
}

// Info : Compute Per particle array of glm::ivec3 (Triangle Indices) particle is part of. 
// This is a scatter approach, each tri index (particle) stores a copy of the tri vert indices and tri index, to per particle tri array.
void Cloth_State::get_particle_trilist()
{
	// Reset per particle inner array tri list. 
	pt_tris.resize(particles.size());
	for (std::vector<glm::ivec4> trilist : pt_tris) trilist.clear();

	// Loop over tris 
	for (std::size_t t = 0; t < tris.size(); ++t)
	{
		glm::ivec3 &tri = tris[t];
		// Add tri ptr to each particle defined by tri indices tri list. 
		for (std::size_t i = 0; i < 3; ++i)
		{
			int p_ind = tri[i];
			// Note we store a copy of the tris vert/particle indices + the tris own index into the ivec4. 
			// Tri (x = vi_0, y = vi_1, z = vi_2, w = t_i) 
			pt_tris[p_ind].emplace_back(tri.x, tri.y, tri.z, t);
		}
	}

#ifdef DEBUG_LOG_PTTRIS
	// Debug : validation
	for (const Particle &p : particles) std::cout << "particle_" << p.id << " tri_count = " << pt_tris[p.id].size() << "\n";
#endif
}

// Info : Built Cloth Spring State
// While this is a slow function due to getting neighbour particles and checking for existing cons, its only ran once (on construction). 
void Cloth_State::build_cloth_springs()
{
	// Get Per Particle Triangle List : 
	get_particle_trilist();
	
	// ============= Build Cloth Springs ==============
	// For each particle
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		Particle &curPt = particles[p];

		// For each tri, particle is part of
		std::vector<glm::ivec4> &triPts = pt_tris[p];
		for (std::size_t t = 0; t < triPts.size(); ++t)
		{
			// For each tri index (Adjacent Particles)
			for (std::size_t i = 0; i < 3; ++i)
			{
				std::size_t ind = triPts[t][i];
				assert(curPt.id == p); // Make sure iter index matches Pt_id. 

				// Get other particle defined by index
				Particle &othPt = particles[ind];
				// Check Spring of these two particles (curPt, othPt) does not already exist 
				bool is_dupe = false;
				for (std::size_t s = 0; s < springs.size(); ++s)
				{
					std::size_t s_p0 = springs[s].pt_0->id, s_p1 = springs[s].pt_1->id;
					if ((s_p0 == curPt.id && s_p1 == othPt.id) || (s_p0 == othPt.id && s_p1 == curPt.id)) is_dupe |= true; 
				}
				// Build Spring for particle pair (if othPt not curPt and Spring is not duplicate)
				if (curPt.id != othPt.id && !is_dupe) 
				{
					// Compute Rest Length 
					float rl = glm::length(curPt.P - othPt.P);

					// Create Spring
					springs.emplace_back(&curPt, &othPt, rl);

					// Increment Cur and Oth Particle Spring Count
					curPt.spring_count++, othPt.spring_count++;
				}
			}
		}
	}

#ifdef DEBUG_LOG_SPRINGS
	// Debug : validation
	for (const Particle &p : particles) std::cout << "particle_" << p.id << " spring_count = " << p.spring_count << "\n";
	std::cout << "Total Spring Count = " << springs.size() << "\n";
#endif
}

// Info : Updates Cloth_Mesh, and call Cloth_Mesh Render. Also will render any additonal visualizers if implemented. 
void Cloth_State::render(const glm::mat4x4 &view, const glm::mat4x4 &persp)
{
	mesh->update_fromParticles();           // Update Mesh (cloth_state --> cloth_mesh)
	mesh->set_cameraTransform(view, persp); // Forward Camera Matrices to cloth_mesh
	mesh->view = view, mesh->persp = persp;
	mesh->render(); 
}

// Info : Reset Cloth to Inital State (Rest Position) and Zero Forces and Velocity. 
void Cloth_State::reset_cloth()
{
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		Particle &curPt = particles[p];
		curPt.P = v_p[p];
		curPt.V.x = 0.f, curPt.V.y = 0.f, curPt.V.z = 0.f; 
		curPt.F.x = 0.f, curPt.F.y = 0.f, curPt.F.z = 0.f;
	}
}

// Info : Set Rest Position Offset 
void Cloth_State::set_rest_offset(const glm::vec3 &offset)
{
	if (offset == rest_offset) return; 

	// Set on both orginal Rest Position array and particle Positions.
	// Remove old rest_offset first

	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		particles[p].P -= rest_offset;
		particles[p].P += offset; 
		v_p[p] -= rest_offset;
		v_p[p] += offset;
	}

	// Store new offset
	rest_offset = offset; 
}

// Info : Assuming cloth is a 2D Grid, fix corner points. 
void Cloth_State::set_fixed_corners(bool state)
{
	// Get dim size
	std::size_t M = static_cast<std::size_t>(std::sqrt(particles.size()));
	if (state)
	{
		particles[0].state = pt_state::FIXED, particles[M-1].state = pt_state::FIXED;
	}
	else
	{
		particles[0].state = pt_state::FREE, particles[M-1].state = pt_state::FREE;
	}
}

// Info : Exports Cloth to Obj Mesh in its current state (single frame), Using Particles-->Vertices mapping, with orginal Indices. 
void Cloth_State::export_mesh(const char *export_path)
{
	std::ofstream out_mesh(export_path);
	if (!out_mesh.is_open())
	{
		std::cerr << "ERROR::Export File Path Not Valid : " << export_path << " !\n";
		return;
	}

	// Write Vertices Section using Particles 
	out_mesh << "# Cloth Mesh Created by COMP5823M Cloth Solver, Student : Niall Horn\n";
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		const Particle &curPt = particles[p];
		out_mesh << "v " << std::fixed << std::setprecision(6) << curPt.P.x << " " << curPt.P.y << " " << curPt.P.z << "\n";
	}

	// Write Normals (Assume each particle-->vert normal is unique)  	
	std::vector<glm::vec3> normals = mesh->calc_normals(); // Fetch Normals from Cloth_Mesh
	for (const glm::vec3 &N : normals) out_mesh << "vn " << std::fixed << std::setprecision(6) << N.x << " " << N.y << " " << N.z << "\n";

	// Write Face Indices
	for (std::size_t t = 0; t < tris.size(); ++t)
	{
		out_mesh << "f ";
		// Format = V_i//N_i
		const glm::ivec3 &tri = tris[t];
		for (std::size_t ti = 0; ti < 3; ++ti)
		{
			out_mesh << tri[ti]+1 << "//" << tri[ti]+1 << " ";
		}
		out_mesh << "\n";
	}

	out_mesh.close();
	std::cout << "DEBUG::Cloth Mesh Sucessfuly Exported ::" << export_path << " !\n";
}
