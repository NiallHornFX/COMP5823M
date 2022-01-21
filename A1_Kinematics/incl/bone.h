#ifndef BONE_H
#define BONE_H

// Std Headers
#include <vector>
#include <tuple>

// Project Headers
#include "mesh.h"


// Class for rendering bones ethier as mesh or lines (primitives). 

class Bone
{
public:
	Bone(glm::vec3 Start, glm::vec3 End, glm::mat4 Trs, std::size_t ID, int32_t Joint_ID); 
	~Bone() = default; 

	 void render(bool Render_Line = false);
	 void set_cameraTransform(const glm::mat4x4 &view, const glm::mat4x4 &persp);

	 //void update(const glm::vec3 &start, const glm::vec3 &end);
	 void update(const glm::mat4 &joint_trs);

public:
	std::size_t bone_id;
	int32_t joint_id; // Joint ID (Joint at starting point of bone) 
	glm::vec3 start, end;
	glm::mat4 transform; 

	Mesh *mesh;
	Primitive *line; 

};

#endif