// COMP5823M - A1 : Niall Horn - camera.cpp

// Implments
#include "camera.h"

// Std Headers
#include <iostream>
#include <algorithm>

// Ext Headers 
#include "ext/GLFW/glfw3.h" // GLFW

// =================== Camera Implementation ===================

Camera::Camera(glm::vec3 pos, float target_offset, float fov, float ar, bool freelook) 
	: Cam_Pos(pos), FOV(fov), Aspect_Ratio(ar), free_look(freelook), Cam_Up(glm::vec3(0.f, 1.f, 0.f))
{
	// Init Camera Bases
	Cam_Basis_Z = glm::normalize(glm::vec3(0.f, 0.f, target_offset)); // Faces out screen (not tgt vector).
	Cam_Basis_X = glm::normalize(glm::cross(Cam_Up, Cam_Basis_Z));
	Cam_Basis_Y = glm::normalize(glm::cross(Cam_Basis_Z, Cam_Basis_X));

	// Override with input based bases
	if (freelook) calc_basis();

	// Set Default Yaw/Pitch Angles - 
	Yaw = 0.0f, Pitch = 0.0f;
	Yaw_Min = -120.0f, Yaw_Max = 120.0f;
	Pitch_Min = -89.0f, Pitch_Max = 89.0f;
	Sensitvity = 0.5f;

	// Set Defualt Perspective Members - 
	Near_Plane = 0.01f;
	Far_Plane = 100.0f;
}

glm::mat4 Camera::get_ViewMatrix()
{
	// Flip Basis for target direction
	return glm::lookAt(Cam_Pos, (Cam_Pos - Cam_Basis_Z), Cam_Up);
}

glm::mat4 Camera::get_PerspMatrix()
{
	return glm::perspective(glm::radians(FOV), Aspect_Ratio, Near_Plane, Far_Plane);
}

void Camera::update_camera(GLFWwindow *window, float Camera_Speed, float dt, float yaw, float pitch)
{
	// ========== Camera Direction Update ==========
	if (free_look && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS))
	{
		Yaw   += yaw   * Sensitvity;
		Pitch += pitch * Sensitvity;
		calc_basis();
	} 

	// Poll 
	glfwPollEvents();

	// ========== Camera Position Update ==========
	
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // Forwards along cam Z
	{
		Cam_Pos -= Cam_Basis_Z * (Camera_Speed * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // Backwards along cam Z
	{
		Cam_Pos += Cam_Basis_Z * (Camera_Speed * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // Left along cam X
	{
		Cam_Pos -= Cam_Basis_X * (Camera_Speed * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // Right along cam X
	{
		Cam_Pos += Cam_Basis_X * (Camera_Speed * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Up along Y
	{
		Cam_Pos += Cam_Basis_Y * (Camera_Speed * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // Down along Y
	{
		Cam_Pos -= Cam_Basis_Y * (Camera_Speed * dt);
	}
}

void Camera::calc_basis()
{
	// Clamp Pitch
	Pitch = std::max(Pitch_Min, std::min(Pitch, Pitch_Max));
	// Clamp Yaw
	//Yaw = std::max(Yaw_Min, std::min(Yaw, Yaw_Max));

	// Calc Target Direction
	float d_x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	float d_y = sin(glm::radians(Pitch));
	float d_z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

	// Update Basis Vectors 
	Cam_Basis_Z = glm::normalize(glm::vec3(d_x, d_y, d_z));
	Cam_Basis_X = glm::normalize(glm::cross(Cam_Up, Cam_Basis_Z));
	Cam_Basis_Y = glm::normalize(glm::cross(Cam_Basis_Z, Cam_Basis_X));
}

std::ostringstream Camera::debug()
{
	std::ostringstream out;
	out << "======== DEBUG::Camera::BEGIN ========\n"
		<< "X = " << "[" << Cam_Basis_X.x << "," << Cam_Basis_X.y << "," << Cam_Basis_X.z << "]\n"
		<< "Y = " << "[" << Cam_Basis_Y.x << "," << Cam_Basis_Y.y << "," << Cam_Basis_Y.z << "]\n"
		<< "Z = " << "[" << Cam_Basis_Z.x << "," << Cam_Basis_Z.y << "," << Cam_Basis_Z.z << "]\n"
		<< "Yaw = " << Yaw << "  Pitch = " << Pitch << "\n"
		<< "Pos = " << "[" << Cam_Pos.x << "," << Cam_Pos.y << "," << Cam_Pos.z << "]\n";
	out << "======== DEBUG::Camera::END ========\n";

	return out; 
}