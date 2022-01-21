// COMP5823M - A3 : Niall Horn - fluid_grid.vert
#version 430 core 

layout (location = 0) in vec3 v_P;
layout (location = 1) in vec3 v_C;

out vec4 vpos; 

void main()
{
	gl_Position = vec4(v_P.x, v_P.y,  v_P.z, 1.0); 
	vpos        = vec4(v_P.x , v_P.y, v_P.z, 1.0); 
}

