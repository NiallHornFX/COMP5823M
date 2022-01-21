// COMP5823M - A3 : Niall Horn - fluid_quad.frag
#version 430 core 

// Input
in vec4 vpos; 
in vec4 gl_FragCoord; 

// Output
out vec4 frag_color; 

// Uniforms
uniform int pt_count;
uniform float min_dens; 
uniform float max_dens;
uniform float max_speed; 
uniform float radius; 
uniform float iso_thresh;

// ========== Particle Data ==========
struct particle
{
	vec2 pos;
	vec2 vel; 
	float dens; 
};
// Particles Struct Array SSBO 
layout(std140, binding = 0) buffer data
{
	particle pts[];  
};

// ========== Util Functions ==========
float fit (float value, float min_a, float max_a, float min_b, float max_b)
{
	return min_b + (value - min_a)*(max_b - min_b) / (max_a - min_a);
}

float meta(vec2 r, float h)
{
	float rl = length(r); 
	if (rl > h) return 0.0;
	float rlh = rl / h;
	return 1.0 - 3.0 * pow(rlh, 2.0) + 3.0 * pow(rlh, 4.0) - pow(rlh, 6.0);
}

void main()
{
	// Map from 0-Window FragCoord_Space to 0-1 UV Space. 
	vec2 uv = (gl_FragCoord.xy - 0) / 1024;
	// Scale to (0-10, XY to match simulation domain space).
	uv *= 10.0; 
	
	// Loop through particles, accum metaball fields
	float val = 0.0;
	float spd;
	int count = 0;
	for (int p = 0; p < pt_count; ++p)
	{
		particle pt = pts[p];
		float rad = fit(pt.dens, min_dens, max_dens, 0.25, 0.35) * radius;
		vec2 r_pt = uv - pt.pos;
		float m = meta(r_pt, rad);  
		val += m;
		// Accumulate Speed for this fragment.
		if (m > 0.1) {spd += length(vec2(pt.vel)); count++;}
	}
	if (val >= iso_thresh) // Iso threshold
	{
		// Speed colour
		spd /= float(count); 
		float t = fit(spd, 0.1, max_speed, 0, 1.0); 
		vec3 col = mix(vec3(0.05, 0.05, 1.0), vec3(1,1,1), t);
		
		frag_color = vec4(col, 1.0); 
		
		//frag_color = vec4(val, val, val, 1.0); 
	} 
}
