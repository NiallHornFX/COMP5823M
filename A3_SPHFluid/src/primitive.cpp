// COMP5823M - A3 : Niall Horn - primitive.cpp

// Implements 
#include "primitive.h"

// Std Headers
#include <iostream>
#include <cassert>

// Ext Headers 
// GLEW
#include "ext/GLEW/glew.h" 
// GLFW
#include "ext/GLFW/glfw3.h" 
// GLM
#include "ext/glm/glm.hpp"


// ================================== Primitive Class Implementation ==================================

Primitive::Primitive(const char *Name)
	: name(Name), vert_count(0), mode(Render_Mode::RENDER_POINTS)
{
	flags.buffers_set  = 0;
	flags.data_set     = 0; 
	flags.shader_set   = 0;

	line_width = 5.f; 
	point_size = 2.f; 

	// Init World to ident
	model = glm::mat4(1);
}

Primitive::~Primitive()
{
	if (!flags.buffers_set) return; 
	if (VAO) glDeleteVertexArrays(1, &VAO);
	if (VBO) glDeleteBuffers(1, &VBO);
}

void Primitive::render()
{
	// Check for state to render
	if (!check_state())
	{
		std::cerr << "ERROR::Primitive::" << name << "::Render called, with incorrectly set state." << std::endl;
		std::terminate();
	}

	// Bind Primitive State
	shader.use();

	// Update Modfied Uniforms
	//shader.setMat4("model", model); // No model in 2D. 

	// Render in set mode
	glBindVertexArray(VAO);
	switch (mode)
	{
		case (RENDER_POINTS) :
		{
			glPointSize(point_size);
			glDrawArrays(GL_POINTS, 0, vert_count);
			break;
		}
		case (RENDER_LINES) :
		{
			glLineWidth(line_width);
			glDrawArrays(GL_LINES, 0, vert_count);
			break;
		}
		case (RENDER_MESH) :
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawArrays(GL_TRIANGLES, 0, vert_count);
			break;
		}
	}

	// Clear State
	glUseProgram(0);
	glBindVertexArray(0);
}

// ======= Info : Mesh Data  =======
// Mesh Data Assumptions 6 * 4 byte floats. 
// Stride :  0         3        
// Data   : (x,y,z) | (r,g,b) | 
// ================================

void Primitive::set_data_mesh(const std::vector<vert> &data)
{
	// Copy Mesh Data
	vert_data.clear();
	vert_count = data.size();

	// Serailize into float array
	for (std::size_t v = 0; v < vert_count; ++v)
	{
		const vert &vert = data[v];
		// Position 
		vert_data.push_back(vert.pos.x), vert_data.push_back(vert.pos.y), vert_data.push_back(vert.pos.z);
		// Colour
		vert_data.push_back(vert.col.r), vert_data.push_back(vert.col.g), vert_data.push_back(vert.col.b);
	}
	flags.data_set = 1;

	// Setup Buffers
	create_buffers();
}
void Primitive::set_data_mesh(const float *data, std::size_t vert_n)
{
	flags.data_set = 0; 
	// Copy Mesh Data
	vert_data.clear();
	vert_data.resize(vert_n * 6);
	vert_count = vert_n;
	std::memcpy(vert_data.data(), data, (vert_n * 6 * sizeof(float)));

	flags.data_set = 1; 

	// Setup Buffers
	create_buffers();
}

void Primitive::create_buffers()
{
	// Delete previous resources 
	if (VAO) glDeleteVertexArrays(1, &VAO);
	if (VBO) glDeleteBuffers(1, &VBO);

	// Gen VAO and VBO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Fill with mesh data (Assumes Mesh is in correct layout within mesh_data float array)
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (vert_count * 6 * sizeof(float)), vert_data.data(), GL_STATIC_DRAW);

	// Vertex Attribute 
	// Position (0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	// Colour (1)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	// Clear bound state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	flags.buffers_set = 1;
}

// These functions could be optimized to only update delta attribute regions of the buffer, layout would need to change.  

// Only sets position of vertices (allows for updating positions per tick)
void Primitive::update_data_position(const std::vector<glm::vec3> &posData)
{
	for (std::size_t v = 0; v < vert_count; ++v)
	{
		std::size_t i = v * 6; // Vert Index, Position. 
		vert_data[i++] = posData[v].x;
		vert_data[i++] = posData[v].y;
		vert_data[i++] = posData[v].z;
	}
	// Refill Buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (vert_count * 6 * sizeof(float)), vert_data.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Only sets colour of vertcies (allows for updating colours per tick)
void Primitive::update_data_colour(const std::vector<glm::vec3> &colData)
{
	for (std::size_t v = 0; v < vert_count; ++v)
	{
		std::size_t i = 3 + v * 6; // Vert Index, Colour. 
		vert_data[i++] = colData[v].r;
		vert_data[i++] = colData[v].g;
		vert_data[i++] = colData[v].b;
	}
	// Refill Buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (vert_count * 6 * sizeof(float)), vert_data.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Update Pos and Colour Data
void Primitive::update_data_position_col(const std::vector<glm::vec3> &posData, const std::vector<glm::vec3> &colData)
{
	for (std::size_t v = 0; v < vert_count; ++v)
	{
		std::size_t P_i = v * 6;     // Vert Index, Position Offset 
		vert_data[P_i++] = posData[v].x, vert_data[P_i++] = posData[v].y, vert_data[P_i++] = posData[v].z;

		std::size_t C_i = 3 + v * 6; // Vert Index, Colour Offset
		vert_data[C_i++] = colData[v].x, vert_data[C_i++] = colData[v].y, vert_data[C_i++] = colData[v].z;
	}
	// Refill Buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (vert_count * 6 * sizeof(float)), vert_data.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Primitive::set_colour(const glm::vec3 &col)
{
	std::vector<glm::vec3> data(vert_count, col);
	update_data_colour(data);
}

void Primitive::set_shader(const char *vert_path, const char *frag_path)
{
	// Create Shader
	std::string shader_name = name + "_Shader";
	shader = Shader(shader_name.c_str(), vert_path, frag_path);

	// Load and build shader
	shader.load();

	// Set Model Matrix
	//shader.setMat4("model", model); // No Model matrix in 2D used. 
	if (shader.valid_state) flags.shader_set = 1; 
}

// Flags that need to be set for rendering to be valid
bool Primitive::check_state() const
{
	return flags.buffers_set & flags.data_set & flags.shader_set;
}

// Model Matrix Transformations 
void Primitive::translate(const glm::vec3 &offs)
{
	model = glm::translate(model, offs);
}

void Primitive::rotate(float d_ang, const glm::vec3 &axis)
{
	model = glm::rotate(model, glm::radians(d_ang), axis);
}

void Primitive::scale(const glm::vec3 &scale)
{
	model = glm::scale(model, scale);
}

// Debug
void Primitive::debug() const
{
	// Check for correct mesh_data size to vertex count with attributes. (11 * sizeof(float) per vert).
	assert((vert_data.size() * sizeof(float)) == (vert_count * sizeof(float) * 11));

	std::cout << "======== DEBUG::Camera::Primitive_" << name << "::Vertex_Data::BEGIN ========\n";
	for (std::size_t v = 0; v < vert_count; ++v)
	{
		std::size_t i = v * 6;
		std::cout << "Vertex_" << v << "\n"
			<< "Pos =  [" << vert_data[i++] << "," << vert_data[i++] << "," << vert_data[i++] << "]\n"
			<< "Col =  [" << vert_data[i++] << "," << vert_data[i++] << "," << vert_data[i++] << "]\n";
	}
	std::cout << "======== DEBUG::Camera::Primitive_" << name << "::Vertex_Data::END ========\n";
}