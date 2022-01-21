#ifndef CAMERA_H
#define CAMERA_H

// Std Headers
#include <sstream>

// Ext Headers 
// GLM
#include "ext/glm/gtc/matrix_transform.hpp"
#include "ext/glm/gtc/type_ptr.hpp"

// FDs
struct GLFWwindow;

// Info : Basic Camera class using GLFW for input polling

class Camera
{
public:
	Camera(glm::vec3 pos, float target_offset, float fov, float ar, bool freelook);
	Camera() {};
	~Camera() = default; 

	// View Members
	glm::vec3 Cam_Pos;
	glm::vec3 Cam_Up;

	// Camera Basis Members
	glm::vec3 Cam_Basis_X;
	glm::vec3 Cam_Basis_Y;
	glm::vec3 Cam_Basis_Z;

	// Mouse Rotation Members - 
	float Yaw, Pitch;
	float Yaw_Min, Pitch_Min;
	float Yaw_Max, Pitch_Max;

	float Sensitvity;
	float FOV, Aspect_Ratio, Near_Plane, Far_Plane;

	bool free_look;

public:
	glm::mat4 get_ViewMatrix();

	glm::mat4 get_PerspMatrix();

	void update_camera(GLFWwindow *window, float Camera_Speed, float dt, float yaw, float pitch);


	std::ostringstream debug();

private:
	void calc_basis();

};

#endif