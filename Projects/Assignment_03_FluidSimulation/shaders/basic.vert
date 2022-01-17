// COMP5823M - A3 : Niall Horn - basic.vert
#version 430 core 

// Input 
layout (location = 0) in vec3 v_P;
layout (location = 1) in vec3 v_C;

// Output
out vec3 colour; 

// Uniform
uniform mat4 model;

void main()
{
	colour = v_C; 
	gl_Position = model * vec4(v_P, 1.0); 
}

