#ifndef BVH_DATA_H
#define BVH_DATA_H

// Std Headers
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

// Ext Headers
#include "ext/glm/glm.hpp"

// Type Def
using real = double; 

using DOF6 = std::tuple<real, real, real, real, real, real>;
//using DOF3 = std::tuple<real, real, real>;
using DOF3 = glm::dvec3; 

// FDs
struct Joint;
struct Channel;

// ==============================================================================
// Info : BVHData Class, responsible for loading, parsing BVH Animation file
//        to Joints and Channel Data. 
// ==============================================================================

class BVH_Data
{
public:
	BVH_Data() = delete;
	BVH_Data(std::string FileName);
	~BVH_Data();

	// Load and Parse BVH File into Joints and Channels
	void Load(); 
	// Reset Data
	void Clear(); 
	// Dump State
	void Debug(bool to_file = false); 

	// Joint Channel Access
	DOF6 get_root_DOF6(std::size_t frame)                               const;
	DOF3 get_joint_DOF3(std::size_t joint_idx, std::size_t frame)       const;
	DOF3 get_joint_offset(std::size_t joint_idx, std::size_t frame = 0) const;

	// Joint Lookup
	Joint* find_joint(const std::string &name) const;


public:

	std::string filename;
	std::size_t num_channel;
	std::size_t num_frame;
	real interval;

	bool write_log; 

private:

	std::vector<Joint*>   joints; 
	std::vector<Channel*> channels; 
	real *motion; 

	std::vector<Joint*> end_sites; 

	friend class Anim_State;
	friend class IK_Solver;
};

// ==============================================================================
// Info : Structs for Joint and Channel Objects 
// ==============================================================================

enum ChannelEnum
{
	X_ROTATION = 0, Y_ROTATION, Z_ROTATION,
	X_POSITION, Y_POSITION, Z_POSITION
};

struct Channel
{
	Joint *joint;            // self joint of channel
	ChannelEnum type;        // DOF type
	std::size_t index;       // index.
};

struct Joint
{
	std::string name;
	std::size_t idx;

	glm::dvec3 offset;   // Relative Joint Offset to parent
	glm::dvec3 end;      // Relative End_Site offset to Joint offset. 
	glm::dvec3 position; // Joint Accumulated Transform Pos (WS)

	Joint *parent;
	std::vector<Joint*> children;
	std::vector<Channel*> channels;

	bool is_root, is_end;
};

#endif