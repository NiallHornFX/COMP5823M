// Implements 
#include "anim_state.h"

#define CREATE_ROOT_BONE 0 

// Default Ctor, state is set on demmand as needed. 
Anim_State::Anim_State()
{
	anim_loop = true;
	anim_frame = 0;
	max_frame = 0;

	bvh = nullptr;
}

void Anim_State::set_bvhFile(const char *BVHPath)
{
	// Clear prev state (could just bvh::clear())
	if (bvh) delete bvh;
	skel.reset();

	// Load BVH File into new BVH_Data
	bvh = new BVH_Data(BVHPath);
	bvh->Load();

	// Update Frame / Time 
	max_frame = bvh->num_frame;
	interval  = bvh->interval;

	// Build Inital Skeleton State
	build_bvhSkeleton();
	update_bvhSkeleton();
}

// Sets Joint Angles for current frame 
void Anim_State::tick()
{
	if (anim_loop) inc_frame();

	// Update Skeleton per tick, from hoint angles. 
	update_bvhSkeleton();
}

// Fill out Skeleton from BVH Tree using offsets. 
// Only needs to be done once per BVH file load, then update joint angles per tick / anim frame.

void Anim_State::build_bvhSkeleton()
{
	// Get root offset  from Channel Data of 0th frame
	// Channel indices should be first 3 (as root is first joint in hierachy)
	glm::vec3 root_offs(bvh->motion[0], bvh->motion[1], bvh->motion[2]);

	for (Joint *joint : bvh->joints)
	{
		if (joint->is_root)
		{
			// Use fetched root offset
			#if CREATE_ROOT_BONE == 1
			skel.add_bone(glm::vec3(0.f), root_offs, glm::mat4(1), -1); // Bone has no starting parent joint (hence -1 index).
			#endif
		}
		else // Regular Joint
		{
			// Get Parent Offset 
			glm::vec3 par_offs(0.f);
			Joint *p = joint;
			while (p->parent)
			{
				// Accumulate offset
				par_offs += p->parent->offset;

				// Traverse up to parent 
				p = p->parent;
			}
			// Add fetched Root offset
			par_offs += root_offs;

			// Add Bone to Skeleton
			skel.add_bone(par_offs, (par_offs + joint->offset), glm::mat4(1), joint->parent->idx); // Bone starts at parent joint. 
		}
	}
}

// Update Bones based on joint data for current set anim_frame. 
void Anim_State::update_bvhSkeleton()
{
	 fetch_traverse(bvh->joints[0], glm::mat4(1.f));
} 

// Recursivly traverse through hierachy, update joints and their resulting bones transforms. 
void Anim_State::fetch_traverse(Joint *joint, glm::mat4 trans)
{
	//  =========== Get Translation  ===========
	if (!joint->parent) // Root joint, translation from channels. 
	{
		glm::vec4 root_offs(0., 0., 0., 1.);

		for (const Channel *c : joint->channels)
		{
			switch (c->type)
			{
				// Translation
				case ChannelEnum::X_POSITION:
				{
					float x_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
					root_offs.x = x_p;
					break;
				}
				case ChannelEnum::Y_POSITION:
				{
					float y_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
					root_offs.y = y_p;
					break;
				}
				case ChannelEnum::Z_POSITION:
				{
					float z_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
					root_offs.z = z_p;
					break;
				}
			}
		}

		trans = glm::translate(trans, glm::vec3(root_offs));
	}
	else if (joint->parent) // Non root joints, Translation is offset. 
	{
		trans = glm::translate(trans, joint->offset);
	}

	// =========== Get Rotation ===========
	glm::mat4 xx(1.), yy(1.), zz(1.);
	for (const Channel *c : joint->channels)
	{
		switch (c->type)
		{
			case ChannelEnum::Z_ROTATION:
			{
				float z_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
				trans = glm::rotate(trans, glm::radians(z_r), glm::vec3(0., 0., 1.));
				break;
			}
			case ChannelEnum::Y_ROTATION:
			{
				float y_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
				trans = glm::rotate(trans, glm::radians(y_r), glm::vec3(0., 1., 0.));
				break;
			}
			case ChannelEnum::X_ROTATION:
			{
				float x_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
				trans = glm::rotate(trans, glm::radians(x_r), glm::vec3(1., 0., 0.));
				break;
			}
		}
	}

	// Store current position. 
	joint->position = glm::vec3(trans * glm::vec4(0.f, 0.f, 0.f, 1.f));

	
	// Search for joint in bones and update transform of each end point.
	for (Bone *bone : skel.bones)
	{
		if (bone->joint_id == joint->idx)
		{
			bone->update(trans); // Pass Joint transform matrix to update bone
		}
	}
	
	/*
	// ==================== End Point ====================
	if (joint->is_end)
	{
		// Search for bone with end point joint and update transform.
		// [..] (not needed for now).
	}
	*/

	// ==================== Children ====================
	// Pass each recurrsive call its own copy of the current (parent) transformations to then apply to children.
	for (std::size_t c = 0; c < joint->children.size(); ++c)
	{
		fetch_traverse(joint->children[c], trans);
	}
}

// ===================== IK Setup =====================
void Anim_State::ik_test_setup()
{
	// Right Arm test. 
	// Get Thumb Pos as Inital Effector location. 
	Joint *r_thumb = bvh->find_joint("RThumb");
	if (!r_thumb) std::terminate;

	Effector *eff_arm_r = new Effector(r_thumb->position, effectors.size());
	eff_arm_r->joints.push_back(r_thumb);
	// Pusback to effectors
	effectors.push_back(eff_arm_r);

	// Traverse back to root, to get joints forming the right arm IK chain. Pass these to IK Solver per tick. 
	// IK Solve

	// Update Joint angles Motion Data for affected joints of IK solve. 
}

// For drawing Anim_States own primtives (eg effectors)
void Anim_State::render()
{
	for (Effector *effec : effectors)
	{
		Primitive *prim = effec->mesh;

		// Scale Model Matrix (Post BVH transform) 
		prim->scale(glm::vec3(0.05f));
		prim->model[3] = prim->model[3] * glm::vec4(0.05f, 0.05f, 0.05f, 1.f); // Also Scale Translation (Post Scale)
		// Set Camera Transform
		prim->set_cameraTransform(view, persp);

		// Render
		prim->render();
	}
}

// ===================== Animation Frame state member functions =====================
// Need to make sure inc/dec is only done for interval of current dt. 

void Anim_State::inc_frame()
{
	anim_frame = ++anim_frame > max_frame ? 0 : anim_frame;
}

void Anim_State::dec_frame()
{
	anim_frame = --anim_frame < 0 ? 0 : anim_frame;
}

void Anim_State::set_frame(std::size_t Frame)
{
	anim_frame = Frame > max_frame ? max_frame : Frame;
}

// ===================== Debug =====================

void Anim_State::debug() const
{
	std::cout << "Anim_State::" << bvh->filename << " ::Frame = " << anim_frame << "\n";
}

void Anim_State::chan_check(std::size_t f) const
{
	// Get random joint
	Joint *joint = bvh->joints[3];

	// Check anim_data over all frames.
	for (std::size_t f = 0; f < bvh->num_frame; ++f)
	{
		std::cout << "Frame " << f << " Data " << bvh->motion[f * bvh->num_channel + joint->channels[0]->index] << "\n";
	}
	
}