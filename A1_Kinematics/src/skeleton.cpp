// Implements
#include "skeleton.h"

Skeleton::Skeleton(const glm::mat4 &TrsRoot)
	: root_transform(TrsRoot)
{
	bone_count = 0;
	render_mesh = false; 
}

Skeleton::Skeleton()
	: root_transform(glm::mat4(1))
{
	bone_count = 0;
	render_mesh = false;
}

// Add Bone, using Joint Offsets and Transformations
// With joint indices, This version also returns pointer to inserted bone,
void Skeleton::add_bone(const glm::vec3 &start, const glm::vec3 &end, const glm::mat4 &trs, std::size_t joint_id)
{
	Bone *b = new Bone(start, end, trs, bone_count + 1, joint_id);
	bones.push_back(b);
	bone_count++;
}


void Skeleton::render(const glm::mat4x4 &view, const glm::mat4x4 &persp)
{
	// Render bones as mesh
	if (render_mesh)
	{
		for (Bone *b : bones)
		{
			b->set_cameraTransform(view, persp);
			b->render(false);
		}
		return; 
	}
	// Render bones as lines 
	for (Bone *b : bones)
	{
		b->set_cameraTransform(view, persp);
		b->render(true);
	}
}

void Skeleton::reset()
{
	bones.clear();
	bone_count = 0;
	root_transform = glm::mat4(1);
}