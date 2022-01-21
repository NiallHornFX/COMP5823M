#ifndef IK_SOLVER_H
#define IK_SOLVER_H

// Std Headers
#include <string>
#include <vector>

// Project Headers
#include "bvhdata.h"
#include "effector.h"
#include "anim_state.h"

// Ext Headers
// GLM
#include "ext/glm/glm.hpp"
// Eigen
#include "ext/Eigen/Eigen"
#include "ext/Eigen/Core"

// Class that handles the Inverse Kinematics Solve given some input joints and effectors. 

class IK_Solver
{
public:
	IK_Solver(Anim_State *Anim, const std::vector<Joint*> &joint_chain, Joint *joint_end, const Effector &target);
	~IK_Solver() = default;

	// ============== Tick / Update ===============
	void tick(); 

	// =============== Build Jacobian ===============
	void  build_jacobian();
	void  perturb_joints();
	void  perturb_traverse(Joint *perturb_joint, ChannelEnum dof);


	// =============== Solve for Angles ===============
	// Pseudo-Inverse Jacobian solve for Joint Angles
	void solve(); 

	// =============== Integrate Angles ===============

	// =============== Debug ===============
	void print_jacobian();

	// =============== Utility Static Member Functions ===============
	static Eigen::Vector3f glmToeig(const glm::vec3 &vec);
	static Eigen::Vector4f glmToeig(const glm::vec4 &vec);
	static Eigen::Vector3d glmToeig(const glm::dvec3 &vec);
	static Eigen::Vector4d glmToeig(const glm::dvec4 &vec);

public:
	std::vector<Joint*> chain; 
	Joint *end_joint; // End Effector / End_Site joint.
	Effector effector_target; 

	std::vector<glm::dvec3> perturb_positons; // Perturbed EndEffector Pos, wrt to each joint DOF. 

	Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *J;  // Jacobian Matrix
	//Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *JI; // Jacobian (Pseudo) Inverse 
	//Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> *DT; // Delta Theta Vector
	Eigen::Vector3d V;                            // Velocity Vector

	//Eigen::Vector3d endEffector_current; 
	//Eigen::Vector3d end_effector_target; 

	glm::dvec3 endEffector_current;

	double perturb_factor; // Perturbation factor / h value used in Jacobian Construction

	Anim_State *anim; // Access to anim_state
};

#endif