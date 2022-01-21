// Implements 
#include "anim_state.h"

// Project Headers
#include "ik_solver.h"

// Std Headers
#include <cassert>
#include <algorithm>

#define CREATE_ROOT_BONE 0 
#define DEBUG_JACOBIAN 1

// Default Ctor, state is set on demmand as needed. 
Anim_State::Anim_State()
{
	anim_loop = true;
	anim_frame = 0;
	max_frame = 0;

	bvh = nullptr;
}

Anim_State::~Anim_State()
{
	// Deallocate singles IK Data
	//if (ik_rightArm)     delete ik_rightArm;
	if (joint_endeffec)  delete joint_endeffec;
	if (target_endeffec) delete target_endeffec;
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

	// Build and Set, inital Skeleton State
	build_bvhSkeleton();
	update_bvhSkeleton();

	// IK Setup
	ik_setup();
	// Initial Tick of IK solvers
	ik_rightArm->tick();
}

// Updates Joint Angles for current frame via Skeleton State
void Anim_State::tick()
{
	// Incrment Anim Frame
	if (anim_loop) inc_frame();

	// Tick IK Solvers for joint chains ...

	// Update Skeleton per tick, from joint angles. 
	update_bvhSkeleton();
}


// ===========================================================================================================
//										Skeleton Inital State Build
// ===========================================================================================================

// =============== Build BVH Skeleton ===============
/* Info : Builds Inital Skeleton of bones, from accumulated joint offset transforms. Resulting in Rest-Pose bones. 
   Only needs to be done once per BVH file load, then update joint angles per tick / anim frame. */
void Anim_State::build_bvhSkeleton()
{
	// Get root offset  from Channel Data of 0th frame
	// Channel indices should be first 3 (as root is first joint in hierachy)
	glm::dvec3 root_offs(bvh->motion[0], bvh->motion[1], bvh->motion[2]);

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
			glm::dvec3 par_offs(0.f);
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

			// Add Bone to Skeleton, bone starts at parent joint. 
			skel.add_bone(par_offs, (par_offs + joint->offset), glm::mat4(1), joint->parent->idx); // 
		}
	}
}

// ===========================================================================================================
//										Skeleton Update (Forward Kinematics)
// ===========================================================================================================

// Update Bones based on joint data for current set anim_frame. 
void Anim_State::update_bvhSkeleton()
{
	 fetch_traverse(bvh->joints[0], glm::dmat4(1.));
} 

// Recursivly traverse through hierachy, update joints and their resulting bones transforms from BVH Motion Data. 
// IK Based joints will be updated through a sepereate method. 
void Anim_State::fetch_traverse(Joint *joint, glm::dmat4 trans)
{
	//  =========== Get Translation  ===========
	if (!joint->parent) // Root joint, translation from channels. 
	{
		glm::dvec4 root_offs(0., 0., 0., 1.);

		for (const Channel *c : joint->channels)
		{
			switch (c->type)
			{
				// Translation
				case ChannelEnum::X_POSITION:
				{
					real x_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
					root_offs.x = x_p;
					break;
				}
				case ChannelEnum::Y_POSITION:
				{
					real y_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
					root_offs.y = y_p;
					break;
				}
				case ChannelEnum::Z_POSITION:
				{
					real z_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
					root_offs.z = z_p;
					break;
				}
			}
		}

		trans = glm::translate(trans, glm::dvec3(root_offs));
	}
	else if (joint->parent) // Non root joints, Translation is offset. 
	{
		trans = glm::translate(trans, joint->offset);
	}

	// =========== Get Rotation ===========
	for (const Channel *c : joint->channels)
	{
		switch (c->type)
		{
			case ChannelEnum::Z_ROTATION:
			{
				real z_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
				trans = glm::rotate(trans, glm::radians(z_r), glm::dvec3(0., 0., 1.));
				break;
			}
			case ChannelEnum::Y_ROTATION:
			{
				real y_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
				trans = glm::rotate(trans, glm::radians(y_r), glm::dvec3(0., 1., 0.));
				break;
			}
			case ChannelEnum::X_ROTATION:
			{
				real x_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
				trans = glm::rotate(trans, glm::radians(x_r), glm::dvec3(1., 0., 0.));
				break;
			}
		}
	}

	// Store current position. 
	joint->position = glm::vec3(trans * glm::dvec4(0., 0., 0., 1.));

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

// ===========================================================================================================
//										Inverse Kinematics Setup 
// ===========================================================================================================

void Anim_State::ik_setup()
{
	// ========== Single Chain setup ==========
	// Joint chain from RThumb (as end joint / end effector), back to root. 
	Joint *end_joint = bvh->find_joint("RThumb");
	if (!end_joint || !end_joint->is_end) // Validate End Joint
	{
		std::cerr << "ERROR::Cannot create joint chain for::" << end_joint->name << "::Joint is not an end joint." << std::endl;
		std::terminate();
	}
	chain_rightArm = create_joint_chain(end_joint);

	// ====== Create Target End Effector from some otjer joint, not part of the chain ======
	Joint *tgt_joint = bvh->find_joint("LThumb");
	if (!tgt_joint) // Validate Target Joint
	{
		std::cerr << "ERROR::Cannot create target effector for::" << tgt_joint->name << "::Joint is not valid." << std::endl;
		std::terminate();
	}
	Effector target(tgt_joint, glm::dvec3(-20.0, 8., 0.));

	// ====== Get Inital Joint Chain Rotational DOFs Motion ======
	// Note : we discard position DOFs for root, will fetch this from BVH Motion data later. 
	// Fetch Joint Rotational DOFs (root --> end)
	for (Joint *joint : chain_rightArm)
	{
		glm::dvec3 dof_rot = bvh->get_joint_DOF3(joint->idx, 0);
		chain_rotMotion.push_back(dof_rot);
	}

	// ====== Pass Joint Chain, End Joint/Effector and Target End Effector to IK Solver ======
	ik_rightArm = new IK_Solver(this, chain_rightArm, end_joint, target);
}

std::vector<Joint*> Anim_State::create_joint_chain(Joint *end_joint, int32_t depth)
{
	// ========= Gather Joints in Chain =========
	// Gather Joints from end joint, if depth -1, gather joints all the way back to root. 
	std::vector<Joint*> chain;
	std::size_t d = 0;
	Joint *joint_i = end_joint;
	while (joint_i->parent)
	{
		chain.push_back(joint_i); // Append to chain

		if (depth > 0 && depth == d) break;

		joint_i = joint_i->parent;
		d++;
	}
	if (depth <= 0) chain.push_back(bvh->joints[0]); // Also add root joint. 

	// Reverse joints order in chain, so root (or joint closest to root) 
	// is first, end_site joint is last. 
	std::reverse(chain.begin(), chain.end());
	return chain; 
}

// ===========================================================================================================
//										Anim State Setters
// ===========================================================================================================
// ToDo : inc/dec only done for anim interval based on current dt. 

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

// ===========================================================================================================
//										DEBUG / TESTS
// ===========================================================================================================

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

// For drawing Anim_States own primtives 
void Anim_State::render(const glm::mat4x4 &view, const glm::mat4x4 &persp)
{
	// Deprecated
}