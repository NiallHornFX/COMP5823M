#ifndef EFFECTOR_H
#define EFFECTOR_H

// Project Headers
#include "primitive.h"
#include "bone.h"

// Ext Headers
#include "ext/glm/glm.hpp"

// Std Headers
#include <vector>

struct Joint;

/* Info : Basic class to define effector of joints, and ability to draw it. 
   This can be used to reprsent both a Chain's End Site / Effector and a Target End Effector. */

// Effector Joint Position should be updated per tick externally via Anim_State.

class Effector
{
public:
	Effector(); 
	Effector(Joint *effector_joint, const glm::dvec3 &Offset);
	~Effector();

	// Update Pos
	void set_pos(const glm::dvec3 &upt_Pos);

	// Render Effector 
	void render(float scale, const glm::mat4x4 &view, const glm::mat4x4 &persp);

public:
	glm::dvec3 pos;               // Current Position (with offset)
	glm::dvec3 offset;            // Target offset.
	std::size_t id; 
	Joint* joint_tgt;            // Effector Joint    

private:
	Mesh *mesh;                  // Draw via mesh
};

#endif