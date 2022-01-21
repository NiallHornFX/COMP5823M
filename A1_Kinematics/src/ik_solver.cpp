// Implements
#include "ik_solver.h"

// Std Headers
#include <sstream>
#include <fstream>

#define LOG_OUTPUT 0

IK_Solver::IK_Solver(Anim_State *Anim, const std::vector<Joint*> &joint_chain, Joint *joint_end, const Effector &target)
	: anim(Anim), end_joint(joint_end)
{	
	// Copy Joint Chain and End Effector Target Ref Ptrs
	chain = joint_chain;
	effector_target = target;
}

void IK_Solver::tick()
{
	// Build Jacobian via Perturbation
	perturb_joints();
	build_jacobian();

	// Inverse Jacobian and Solve for Joint Angle Deltas

	// Integrate Joint Angle Deltas to Joint Chain via FK + Forward Euler. 
}

// ===========================================================================================================
//										Jacobian Construction via Joint Perturbation 
// ===========================================================================================================

// ============================= Perturb Joints : Base Function =============================
/* Info : Perturbs each joints DOF while hold the rest constant, to calculate each P2 column entry for the Jacobian.
		  This is the FDM Numerical solve of partial derivatives of the end effector wrt each joint angle. 
*/
void IK_Solver::perturb_joints()
{
	// Start of chain, is assumed to be root. 
	Joint *root = anim->bvh->joints[0];

	// Number of Columns (theta_0...theta_n) (size = joints * 3dof) 
	std::size_t dof_c = chain.size() * 3;

	// Vector to store preturbed postions of end effector, for each perturbed joint, DOF. For each column of J. 
	perturb_positons.reserve(dof_c);

	// Also store the current ticks orginal (non per-turbed) end effector postion. 
	endEffector_current = end_joint->position;

	// Delta Theta to perturb joint DOFs by FDM : (P2(theta+h) - P1(theta) / h). 
	perturb_factor = 0.001f;

	// For each joint, for each DOF, traverse the chain hierachy, perturbing only the cur DOF, and then deriving
	// the resulting end site postion , Note that end site position == end joint/effector postion as there is no 
	// offset on the end_site locations. 
	for (Joint *perturb_joint : chain)
	{
		glm::vec3 org_pos = perturb_joint->position; // Orginal Pos before perturbation of joint. 

		for (std::size_t c = 0; c < 3; ++c) // 0-2 (X-Z rotation DOFs)
		{
			ChannelEnum DOF = static_cast<ChannelEnum>(c); // Get current DOF to peturb, for joint. 

			// Reset Joint Pos back to orginal (to remove last perturbation)
			perturb_joint->position = org_pos;

			// Traverse start of joint chain (assumed to be root), when we reach the preturb joint, we perturb its cur DOF. 
			// Accumulate the resulting transform along the chain and store the resulting end effector / end joint position. 
			perturb_traverse(perturb_joint, DOF);

			// Query Resulting Perturbed End Effector Postion component
			glm::dvec3 P2 = end_joint->position;
			// Append to P2 Column Array
			perturb_positons.push_back(std::move(P2));
		}
	}
}

// ============================= Perturb Joints : Traversal =============================
/* Info : Traversal of Joint Chain Hierachy, applying perturbations to specified DOF on specified joint. 
		  This is done as an iterative loop along the chain. Non perturbed joints are held constant, the
		  accumulated transform are passed forward to the child, untill we reach (and modifiy) the end effector pos. 
   Note : Possible via Friend Class of BVHData and Anim_State
*/
void IK_Solver::perturb_traverse(Joint *perturb_joint, ChannelEnum dof)
{
	glm::dmat4 trans(1.f);                                 // Accumulated Transform
	std::size_t anim_frame = anim->anim_frame;             // Current Animation Frame;
	std::size_t num_channel = anim->bvh->num_channel;      // BVH Number of Motion Channels
	real *bvh_motion = anim->bvh->motion;                  // BVH Motion Data Ptr

	std::size_t j_idx = 0;
	for (Joint *joint : chain)
	{
		//  =========== Translation Still Fetches from BVH Joint Data =========== 
		if (joint->is_root) // Root joint, translation from channels. 
		{
			glm::vec4 root_offs(0., 0., 0., 1.);
			for (const Channel *c : joint->channels)
			{
				switch (c->type)
				{
					// Joint Translation still fetched from BVH
					case ChannelEnum::X_POSITION:
					{
						float x_p = bvh_motion[anim_frame * num_channel + c->index];
						root_offs.x = x_p;
						break;
					}
					case ChannelEnum::Y_POSITION:
					{
						float y_p = bvh_motion[anim_frame * num_channel + c->index];
						root_offs.y = y_p;
						break;
					}
					case ChannelEnum::Z_POSITION:
					{
						float z_p = bvh_motion[anim_frame * num_channel + c->index];
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

		// ====== Perturbation of Joints DOF's OR Hold Constant ======
		// Joint Rotation DOFs, come from Joint Chain Rotional DOF Data Array. 
		glm::dvec3 j_rot = anim->chain_rotMotion[j_idx];

		if (joint->idx == perturb_joint->idx) // Perturbed Joint Rotation 
		{
			for (const Channel *c : joint->channels)
			{
				switch (c->type)
				{
					case ChannelEnum::Z_ROTATION:
					{
						real z_r = j_rot.z; 
						// Preturb for DOF Rot_Z
						if (dof == ChannelEnum::Z_ROTATION) z_r += perturb_factor;
						trans = glm::rotate(trans, glm::radians(z_r), glm::dvec3(0., 0., 1.));
						break;
					}
					case ChannelEnum::Y_ROTATION:
					{
						real y_r = j_rot.y; 
						// Preturb for DOF Rot_Y
						if (dof == ChannelEnum::Y_ROTATION) y_r += perturb_factor;
						trans = glm::rotate(trans, glm::radians(y_r), glm::dvec3(0., 1., 0.));
						break;
					}
					case ChannelEnum::X_ROTATION:
					{
						real x_r = j_rot.x; 
						// Preturb for DOF Rot_X
						if (dof == ChannelEnum::X_ROTATION) x_r += perturb_factor;
						trans = glm::rotate(trans, glm::radians(x_r), glm::dvec3(1., 0., 0.));
						break;
					}
				}
			}
		}
		else // Constant Joint Rotation for all DOFs
		{
			for (const Channel *c : joint->channels)
			{
				switch (c->type)
				{
					case ChannelEnum::Z_ROTATION:
					{
						real z_r = j_rot.z;
						trans = glm::rotate(trans, glm::radians(z_r), glm::dvec3(0., 0., 1.));
						break;
					}
					case ChannelEnum::Y_ROTATION:
					{
						real y_r = j_rot.y;
						trans = glm::rotate(trans, glm::radians(y_r), glm::dvec3(0., 1., 0.));
						break;
					}
					case ChannelEnum::X_ROTATION:
					{
						real x_r = j_rot.x;
						trans = glm::rotate(trans, glm::radians(x_r), glm::dvec3(1., 0., 0.));
						break;
					}
				}
			}
		}
		// ONLY transform the end_site, we don't care about actually transforming the other joints aslong as we have their
		// transforms accumulated (to propgate to the end site joint as it is what we are measuring the delta of). 
		if (joint->is_end) 
		{
			joint->position = glm::vec3(trans * glm::vec4(0.f, 0.f, 0.f, 1.f));
		}

		j_idx++; // Joint Index inc
	}
}

void IK_Solver::build_jacobian()
{
	// ============================ Get UnPerturbed Joint End Effector Postions ============================
	// Get Un-perturbed Joint End Effector Position (P1) 
	//endEffector_current = end_joint->position;

	// ============================ Get Perturbed Effector Postions wrt Joint DOF ============================
	// Perturb Joints to calc partial(x) / partial(theta) of joint end effector wrt joint rotiational DOFs. 
	perturb_joints();

	// ============================ Allocate Jacobian Matrix ============================
	std::size_t rows = 3; // 3 End Effector DOFs
	std::size_t cols = chain.size() * 3; // Joint Count * 3 Rot DOFs.
	J = new Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
	J->resize(rows, cols);
	J->setZero();

	// ============================ Build Jacobian Column Wise ============================
	// In form of : Forward Difference (P2(theta + h) - P1(theta) / h)
	std::size_t col_ind = 0; // Column / Joint DOF index. 
	for (auto col : J->colwise())
	{
		// Each Column get Non-Perturbed (P1) and Perturbed (P2) end effector postion vector3s. 
		Eigen::Vector3d P1 = glmToeig(endEffector_current);
		Eigen::Vector3d P2 = glmToeig(perturb_positons[col_ind]);

		// Split into Effector Positional components (x,y,z) form ((P2 - P1) / h) for each el. 
		col(0) = (P2.x() - P1.x()) / perturb_factor;
		col(1) = (P2.y() - P1.y()) / perturb_factor;
		col(2) = (P2.z() - P1.z()) / perturb_factor;

		col_ind++;
	}

	print_jacobian();
}



// ===========================================================================================================
//										Jacobian Inverse & Solve for Angle Delta
// ===========================================================================================================
void IK_Solver::solve()
{
	// ============================ Compute RHS V ============================
	V = glmToeig(effector_target.pos) - glmToeig(end_joint->position);
	
	// ============================ (Pseudo) Inverse of Jacobian ============================
	// Pinv using Transpose 
	//JI = new Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;
	auto J_Transpose = J->transpose();

	// Joint DOF Angle Vector
	//DT = new Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
	//DT->resize(J->cols(), 1);

	auto DT_a = J_Transpose * V;
}



// ===========================================================================================================
//										Integration of Delta Joint Angles
// ===========================================================================================================
// Use Hardcoded Joint Chain Angles for RightArm chain within Anim_State. 
// Ideally extend to multiple arrays / chains of angles to integrate. 




// ===========================================================================================================
//										Utility, Debug & Static Member Functions
// ===========================================================================================================

// glm to eigen conversions
Eigen::Vector3f IK_Solver::glmToeig(const glm::vec3 &vec)
{
	return Eigen::Vector3f(vec.x, vec.y, vec.z);
}

Eigen::Vector3d IK_Solver::glmToeig(const glm::dvec3 &vec)
{
	return Eigen::Vector3d(vec.x, vec.y, vec.z);
}

Eigen::Vector4f IK_Solver::glmToeig(const glm::vec4 &vec)
{
	return Eigen::Vector4f(vec.x, vec.y, vec.z, vec.w);
}

Eigen::Vector4d IK_Solver::glmToeig(const glm::dvec4 &vec)
{
	return Eigen::Vector4d(vec.x, vec.y, vec.z, vec.w);
} 

// ToDo : Add log output 
void IK_Solver::print_jacobian()
{
	std::stringstream r0, r1, r2, out;
	for (auto col : J->colwise())
	{
		r0 << col(0) << ", "; r1 << col(1) << ", "; r2 << col(2) << ", ";
	}
	out << "\n======== DEBUG::JACOBIAN_MATRIX::BEGIN ========\n"
		<< "Frame = " << anim->anim_frame << "  Rows = " << J->rows() << " Cols = " << J->cols() << "\n"
		<< "|" << r0.str() << "|\n" << "|" << r1.str() << "|\n" << "|" << r2.str() << "|\n";
	out << "======== DEBUG::JACOBIAN_MATRIX::END ========\n\n";

#if LOG_OUTPUT == 1
	std::ofstream log("log.txt", std::ios::out | std::ios::app);
	log << out.str();
	log.close();
#else
	std::cout << out.str(); 
#endif

}