// COMP5823M - A3 : Niall Horn - collider.vert
#version 400 core 

// Input 
layout (location = 0) in vec3 v_P;
layout (location = 1) in vec3 v_C;

// Output
out vec3 colour; 

// Uniforms
uniform mat4 proj; 

void main()
{
	colour = v_C; 
	gl_Position = proj * vec4(v_P, 1.0);
}

