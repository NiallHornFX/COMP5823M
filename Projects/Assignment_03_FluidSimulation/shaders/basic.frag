// COMP5823M - A3 : Niall Horn - colour.frag
#version 430 core 

in vec3 colour;
out vec4 frag_colour; 

void main()
{
	frag_colour = vec4(colour, 1.0);
}
