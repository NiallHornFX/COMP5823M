// COMP5823M - A3 : Niall Horn - collider.frag
#version 400 core 

// Input
in vec3 colour;
// Output
out vec4 frag_colour; 

void main()
{
	frag_colour = vec4(colour, 1.0);
}
