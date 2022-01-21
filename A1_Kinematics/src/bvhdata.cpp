// Implements
#include "bvhdata.h"

// Std Headers
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cassert>

#define  BUFFER_LENGTH  1024*4

// Info : BVH Loader based on code by Masaki OSHITA (www.oshita-lab.org)

BVH_Data::BVH_Data(std::string FileName) 
	: filename(FileName)
{
	// Init
	write_log = false; 
	motion = nullptr;
	Clear();
}

BVH_Data::~BVH_Data()
{
	// Clear Channels and Joints
	for (std::size_t i = 0; i < channels.size(); ++i) delete channels[i];
	for (std::size_t i = 0; i < joints.size(); ++i)   delete joints[i];

	// Clear Motion data
	if (motion) { delete[] motion; motion = nullptr; }
}


void BVH_Data::Load()
{
	char              line[BUFFER_LENGTH];
	char*             token;
	char              separater[] = " :,\t";
	bool              is_site = false;
	double			  x, y, z;
	std::size_t       i, j;
	std::vector<Joint*>  joint_stack; // Reccursive Join stack
	Joint*    joint     = nullptr;
	Joint*    new_joint = nullptr;

	// Clear Current State
	Clear();

	std::ifstream file(filename);
	// Check file exists
	if (!file.is_open())
	{
		std::cerr << "ERROR::BVH_Data could not load file" << std::endl;
		return;
	}

	// Loading 
	std::cout << "INFO::Loading BVH File " << filename << " started.\n";

	while (!file.eof())
	{
		if (file.eof())  goto bvh_error;

		// Get single word token.
		file.getline(line, BUFFER_LENGTH);
		token = strtok(line, separater);
		if (token == NULL)  continue;

		// Start of Joint Block
		if (strcmp(token, "{") == 0)
		{
			joint_stack.push_back(joint);
			joint = new_joint;
			continue;
		}

		// End of Joint Block
		if (strcmp(token, "}") == 0)
		{
			joint = joint_stack.back();
			joint_stack.pop_back();
			is_site = false;
			continue;
		}

		// Start of Joint Information
		if ((strcmp(token, "ROOT") == 0) || (strcmp(token, "JOINT") == 0))
		{
			// New Joint
			new_joint = new Joint();
			new_joint->idx = joints.size();
			new_joint->parent = joint;
			// Set if end (end site)
			new_joint->is_end = false;
			// Set if root
			if (strcmp(token, "ROOT") == 0) new_joint->is_root = true; else new_joint->is_root = false; 
			new_joint->offset[0] = 0.0;  new_joint->offset[1] = 0.0;  new_joint->offset[2] = 0.0;

			// Append to joint array
			joints.push_back(new_joint);

			// If valid parent, add self as child joint. 
			if (joint) joint->children.push_back(new_joint);

			token = strtok(NULL, "");
			while (*token == ' ')  token++;
			new_joint->name = token;

			//joint_index[new_joint->name] = new_joint;
			continue;
		}

		// Start of end information
		if ((strcmp(token, "End") == 0))
		{
			new_joint = joint;
			is_site = true;
			continue;
		}

		// Joint Offset (or end position) 
		if (strcmp(token, "OFFSET") == 0)
		{
			token = strtok(NULL, separater);
			x = token ? atof(token) : 0.0;
			token = strtok(NULL, separater);
			y = token ? atof(token) : 0.0;
			token = strtok(NULL, separater);
			z = token ? atof(token) : 0.0;

			joint->is_end = is_site;
			if (is_site)
			{
				joint->end.x = x;
				joint->end.y = y;
				joint->end.z = z;
			}
			else
			{
				joint->offset.x = x;
				joint->offset.y = y;
				joint->offset.z = z;
			}
		
			continue;
		}

		// Joint Channel info
		if (strcmp(token, "CHANNELS") == 0)
		{
			token = strtok(NULL, separater);

			// Resize channels of joint
			joint->channels.resize(token ? atoi(token) : 0);

			// Create Indvidual Channels
			for (i = 0; i < joint->channels.size(); i++)
			{
				Channel *channel = new Channel();
				channel->joint = joint;
				channel->index = channels.size();

				// Set Channel type 
				token = strtok(NULL, separater);
				if (strcmp(token, "Xrotation") == 0)
					channel->type = X_ROTATION;
				else if (strcmp(token, "Yrotation") == 0)
					channel->type = Y_ROTATION;
				else if (strcmp(token, "Zrotation") == 0)
					channel->type = Z_ROTATION;
				else if (strcmp(token, "Xposition") == 0)
					channel->type = X_POSITION;
				else if (strcmp(token, "Yposition") == 0)
					channel->type = Y_POSITION;
				else if (strcmp(token, "Zposition") == 0)
					channel->type = Z_POSITION;

				// Store Channel in BVH_Data Array
				channels.push_back(channel);
				// Store Channel ptr on in joint channel array. 
				joint->channels[i] = channel; 
			}
		}

		// Break into Motion Section Parsing
		if (strcmp(token, "MOTION") == 0)
			break;
	}

	// Motion Data Parsing
	file.getline(line, BUFFER_LENGTH);
	token = strtok(line, separater);
	if (strcmp(token, "Frames") != 0)  goto bvh_error;
	token = strtok(NULL, separater);
	if (token == NULL)  goto bvh_error;
	// Get Number of frames. (else bvh_error assumes file structure is incorrect)
	num_frame = atoi(token);

	file.getline(line, BUFFER_LENGTH);
	token = strtok(line, ":");
	if (strcmp(token, "Frame Time") != 0)  goto bvh_error;
	token = strtok(NULL, separater);
	if (token == NULL)  goto bvh_error;
	// Get Time Iterval (else bvh_error assumes file structure is incorrect)
	interval = atof(token);

	// Set Numer of Channels
	num_channel = channels.size();

	// Allocate Motion Data (Number of Frames * Number of Channels)
	motion = new double[num_frame * num_channel];

	// Loop per frame and store channel data
	for (i = 0; i < num_frame; i++)
	{
		file.getline(line, BUFFER_LENGTH);
		token = strtok(line, separater);
		for (j = 0; j < num_channel; j++)
		{
			if (token == NULL) goto bvh_error;

			// Set frame channel data
			motion[i*num_channel + j] = atof(token);

			token = strtok(NULL, separater);
		}
	}

	file.close();

	// Debug / Write Log 
	if (write_log) Debug(true);

	// Store End Site Joints ptrs. 
	for (Joint *joint : joints)
	{
		if (joint->is_end) end_sites.push_back(joint);
	}

	// Debug tmp
	std::cout << "INFO::Loading BVH File " << filename << " Completed ! \n";

	// Finished
	return;

bvh_error:
	file.close();
	std::cerr << "ERROR::Loading BVH File" << std::endl;
}

void BVH_Data::Clear()
{
	// Clear Channels and Joints
	for (std::size_t i = 0; i < channels.size(); ++i) delete channels[i];
	for (std::size_t i = 0; i < joints.size(); ++i) delete joints[i];

	// Clear Motion data
	if (motion) {delete[] motion; motion = nullptr;}

	// Clear Arrays
	channels.clear();
	joints.clear();

	// Reset
	num_channel = 0;
	num_frame = 0; 
	interval = 0.;
}

// Get Rotational Motion Data for some Joint, for some frame. 
// Returns in (x,y,z) order so not to confuse indexing with DOF3 = glm::dvec3 (x,y,z)
DOF3 BVH_Data::get_joint_DOF3(std::size_t joint_idx, std::size_t frame) const
{
	// Channel count is 3 for 3DOF (ZRot, YRot, XRot) (3DOF) unless root 6DOF (XPos, YPos, ZPos, ZRot, YRot, XRot).
	// DEBUG CODE (Remove me !) Check Channel Ordering is correct as per above
	//assert((joint->channels[0]->type == ChannelEnum::Z_ROTATION));
	//assert((joint->channels[1]->type == ChannelEnum::Y_ROTATION));
	//assert((joint->channels[2]->type == ChannelEnum::X_ROTATION));

	Joint *joint = joints[joint_idx];

	// Get Channel, frame, from motion data array using index offset :
	// (frame * numchannel * channelidx) : 
	if (joint->is_root)
	{
		// Should be ordered (Z,Y,X), first 3 for root are translational. 
		std::size_t z_rot_idx = joint->channels[3]->index;
		std::size_t y_rot_idx = joint->channels[4]->index;
		std::size_t x_rot_idx = joint->channels[5]->index;

		real z_rot = motion[frame * num_channel + z_rot_idx];
		real y_rot = motion[frame * num_channel + y_rot_idx];
		real x_rot = motion[frame * num_channel + x_rot_idx];

		return DOF3(x_rot, y_rot, z_rot);
	}
	else
	{
		// Should be ordered (Z,Y,X)
		std::size_t z_rot_idx = joint->channels[0]->index;
		std::size_t y_rot_idx = joint->channels[1]->index;
		std::size_t x_rot_idx = joint->channels[2]->index;

		real z_rot = motion[frame * num_channel + z_rot_idx];
		real y_rot = motion[frame * num_channel + y_rot_idx];
		real x_rot = motion[frame * num_channel + x_rot_idx];

		return DOF3(x_rot, y_rot, z_rot);
	}
}

DOF6 BVH_Data::get_root_DOF6(std::size_t frame) const
{
	Joint *root = joints[0];
	// Root should be the first joint. 
	if (!root->is_root) std::terminate(); 

	// Channel Order for 6DOF Should be : (XPos, YPos, ZPos, ZRot, YRot, XRot)
	// Translation Channel Indices
	std::size_t x_trs_idx = root->channels[0]->index;
	std::size_t y_trs_idx = root->channels[1]->index;
	std::size_t z_trs_idx = root->channels[2]->index;
	// Rotation Channel Indices
	std::size_t z_rot_idx = root->channels[3]->index;
	std::size_t y_rot_idx = root->channels[4]->index;
	std::size_t x_rot_idx = root->channels[5]->index;

	// Return 6DOF Channels of Root Joint : 
	real x_trs = motion[frame * num_channel + x_trs_idx];
	real y_trs = motion[frame * num_channel + y_trs_idx];
	real z_trs = motion[frame * num_channel + z_trs_idx];

	real z_rot = motion[frame * num_channel + z_rot_idx];
	real y_rot = motion[frame * num_channel + y_rot_idx];
	real x_rot = motion[frame * num_channel + x_rot_idx];

	// Remember that DOF6 order is (X_trs, Y_trs, Z_trs, Z_rot, Y_rot, X_rot)
	return DOF6(x_trs, y_trs, z_trs, z_rot, y_rot, x_rot);
}

DOF3 BVH_Data::get_joint_offset(std::size_t joint_idx, std::size_t frame) const
{
	if (joint_idx == 0) // Root Joint, use Channel Translation Data
	{
		real x_trs = motion[frame * num_channel + 0];
		real y_trs = motion[frame * num_channel + 1];
		real z_trs = motion[frame * num_channel + 2];
		return glm::dvec3(x_trs, y_trs, z_trs);
	}
	// Non Root Joint use offset.
	return joints[joint_idx]->offset;
}

// Find Joint by name string
Joint* BVH_Data::find_joint(const std::string &name) const
{
	for (Joint *joint : joints)
	{
		if (joint->name == name) return joint; 
	}
	return nullptr; 
}

void BVH_Data::Debug(bool to_file)
{
	std::stringstream out;
	
	out << "======== DEBUG::BVH_Data::BEGIN========\n"
		<< "File Name = " << filename << "\n"
		<< "Joint Count = " << joints.size() << "\n"
		<< "Channel Count = " << channels.size() << "\n";
		out << "\n======== Joint Data ========\n";
		for (const Joint *j : joints)
		{
			const char* is_root = j->is_root ? " True " : " False ";
			const char* is_end  = j->is_end ?  " True " : " False ";
			out << "Info:: Joint = " << j->idx << " | Name = " << j->name << " | Channels = "
				<< j->channels.size() << " | Children = " << j->children.size() << " | Is Root = " << is_root
				<< " | Is End  = " << is_end << "\n";
		}
		out << "\n======== Channel + Motion Data ========\n";
		for (const Channel *c : channels)
		{
			out << "===== Channel_" << c->index << " Data::BEGIN =====\n";
			// Get Channel Motion Data per frame : 
			for (std::size_t f = 0; f < num_frame; ++f)
			{
				out << "Frame = " << f << " | Data = " << std::fixed << std::setprecision(4) << motion[f * num_channel + c->index] << "\n";
			}
			out << "===== Channel_" << c->index << " Data::END =====\n";
 		}
	out << "\n======== DEBUG::BVH_Data::END========" << std::endl;

	if (to_file)
	{
		std::ofstream log("log.txt"); 
		out >> log.rdbuf(); 
		return; 
	}
	std::cout << out.rdbuf();
}