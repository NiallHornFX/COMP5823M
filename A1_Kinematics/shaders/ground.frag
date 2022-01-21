#version 430 core 

#define debug 0

in vec3 colour; 
in vec3 poscol; 
in vec2 uv; 

out vec4 frag_colour; 

uniform sampler2D tex; 

void main()
{
	#if debug == 0
	// Sample Grid Texture
	vec4 tex_sample = texture(tex, uv);
	// Discard cells
	if (tex_sample.x < 0.05) discard;
	float c = tex_sample.x; 
	
	// Alpha by depth
	float a = c;
	a *= (2.0 - poscol.z);
	
	// Render Texture Grid 
	frag_colour = vec4(vec3(c), a);
	
	#else
	// Render UV Coords 
	frag_colour = vec4(uv.x, uv.y, 0.0, 1.0);
	#endif
}
