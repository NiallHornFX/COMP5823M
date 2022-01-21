#ifndef ANIM_STATE_H
#define ANIM_STATE_H

// Std Headers
#include <map>
#include <tuple>

// Ext Headers
// Eigen
#include "ext/Eigen/Eigen"
#include "ext/Eigen/Core"

// Project Headers
#include "skeleton.h"
#include "bvhdata.h"
#include "effector.h"

class IK_Solver; 

// Info :  Class where animation processing is based (BVH, FK, IK). Exists within app as Tick stage. 

class Anim_State
{
public:
	// ===== Ctor / Dtor =====
	Anim_State();
	~Anim_State();

	// ===== Core =====
	void tick();
	void render(const glm::mat4x4 &view, const glm::mat4x4 &persp);

	// ===== BVH Data =====
	void set_bvhFile(const char *BVHPath);

	// ===== Anim Controls =====
	void set_frame(std::size_t Frame);
	void inc_frame();
	void dec_frame();

	// ===== Skeleton (Bones) Build and Update =====
	void build_bvhSkeleton(); // Build Skeleton from joints, inital state.
	void update_bvhSkeleton(); // Update Skeleton from joints, per tick.
	void fetch_traverse(Joint *joint, glm::dmat4 trans);

	// ===== IK Setup =====
	void ik_setup(); 
	std::vector<Joint*> create_joint_chain(Joint *end_joint, int32_t depth = -1); 

	//void ik_test_tick();
	//void ik_apply_deltas(const Eigen::Matrix<float, Eigen::Dynamic, 1> &deltas); 
	//std::vector<std::pair<glm::vec3, glm::vec3>> perturb_joints(std::vector<Joint*> &chain, Joint *end_effec, Joint *start_joint, float perturb_factor); 
	//void perturb_traverse(std::vector<Joint*> &chain, Joint *perturb_joint, ChannelEnum dof, float perturb_fac); 

	// ===== Debug =====
	void debug() const;
	void chan_check(std::size_t f) const; 

public:

	// ===== BVH Data =====
	BVH_Data *bvh;

	// ===== IK Chain =====
	// Right Arm IK 
	std::vector<Joint*> chain_rightArm;
	// Chain Angle Motion
	std::vector<glm::dvec3> chain_rotMotion;
	// ===== IK Data =====
	IK_Solver *ik_rightArm;
	Joint    *joint_endeffec;
	Effector *target_endeffec;

	// ===== Skeleton =====
	Skeleton skel;

	// ===== Anim Playback State =====
	std::size_t anim_frame, max_frame; 
	float interval; 
	bool anim_loop;

	// ===== Viewer Data =====
	float viewer_dt; 
	std::size_t viewer_tick_c; 
};


#endif