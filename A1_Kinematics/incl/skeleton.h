#ifndef SKELETON_H
#define SKELETON_H

// Std Headers
#include <vector>

// Project Headers
#include "bone.h"

// Info : Container class, holds array of bones between joints to render ethier as mesh or lines using computed transforms,
// from ethier FK or IK joints. 

class Skeleton
{
public:
	Skeleton(const glm::mat4 &TrsRoot);
	Skeleton();
	~Skeleton() = default; 

	void add_bone(const glm::vec3 &start, const glm::vec3 &end, const glm::mat4 &trs, std::size_t joint_id);

	void render(const glm::mat4x4 &view, const glm::mat4x4 &persp);

	void reset();

public:

	bool render_mesh;
	std::size_t bone_count;
	glm::mat4 root_transform;

private:
	std::vector<Bone*> bones;

	friend class Anim_State; 
};


#endif