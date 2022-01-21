// Implements
#include "effector.h"

// Std Headers
#include <string>

// Project Headers
#include "bvhdata.h"

Effector::Effector(Joint *effector_joint, const glm::dvec3 &Offset)
	: joint_tgt(effector_joint), offset(Offset)
{
	// Effector inital state
	pos = joint_tgt->position + offset;
	
	// ========== Create Effector Mesh Prim ==========
	std::string name = "Effector_Mesh_" + std::to_string(joint_tgt->idx);
	mesh = new Mesh(name.c_str(), "../../assets/mesh/sphere.obj");
	mesh->load_obj(false);
	mesh->set_shader("../../shaders/basic.vert", "../../shaders/colour.frag");
	mesh->set_colour(glm::vec3(1.f, 0.f, 0.f));
	// Radius Scale
	mesh->scale(glm::vec3(0.05f, 0.05f, 0.05f));
	// Translate to effector starting position 
	mesh->translate(pos);
}

Effector::Effector()
{
	joint_tgt = nullptr; 
}

Effector::~Effector()
{
	if (mesh) delete mesh; 
}

// Re-Set Effector position from user interaction (adding to offset)
void Effector::set_pos(const glm::dvec3 &updt_Pos)
{
	// Reset Mesh Translation
	mesh->translate(-pos);
	offset += updt_Pos;
	pos = joint_tgt->position + glm::dvec3(offset.x, offset.y, offset.z);
	mesh->translate(pos);
}

// Render Effector Mesh 
void Effector::render(float scale, const glm::mat4x4 &view, const glm::mat4x4 &persp)
{
	mesh->model[3] = glm::vec4((glm::vec3(pos.x, pos.y, pos.z)* glm::vec3(scale)), 1.f);

	// Set Camera Transform
	mesh->set_cameraTransform(view, persp);

	// Render
	mesh->render();
}