// COMP5823M - A2 : Niall Horn - cloth_wire.frag
#version 400 core 

// Output 
out vec4 frag_colour; 

uniform bool wire; 

void main()
{
	if (wire) 
	{
		frag_colour = vec4(vec3(0.1, 0.1, 0.1), 1); 
	}
	else
	{
		frag_colour = vec4(vec3(0.1, 0.1, 0.75), 1); 
	}
}
