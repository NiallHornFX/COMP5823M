// COMP5823M - A3 : Niall Horn - fluid_grid.frag
#version 400 core 

// Input
in vec4 vpos; 
in vec4 gl_FragCoord; 

// Output
out vec4 frag_color; 

// Grid Texture Samplers - 
uniform sampler2D d_tex;       // Texture Unit 0 : Density    (Scalar R) 
uniform sampler2D v_u_tex;     // Texture Unit 1 : Velocity_u (Scalar R)
uniform sampler2D v_v_tex;     // Texture Unit 2 : Veloicty_v (Scalar R) 

// Constant Uniforms - 
uniform int N_Size; // Number of cells per dim. 

// ====== Util Functions ====== 
float fit (float value, float min_a, float max_a, float min_b, float max_b)
{
	return min_b + (value - min_a)*(max_b - min_b) / (max_a - min_a);
}

float deg2rad (int deg)
{
	float PI = 3.14159265;
	return deg * PI / 180.0; 
}

float rand(float n)
{
	return fract(sin(n) * 43758.5453123);
}

void main()
{
	// Map from 0-Window FragCoord_Space to 0-1 UV Space. 
	vec2 uv = (gl_FragCoord.xy - 0) / 1024;
	
	float dens =  clamp(texture(d_tex, uv),   0.0, 1.0).r; 
	float vel_x = clamp(texture(v_u_tex, uv), 0.0, 1.0).r; 
	float vel_y = clamp(texture(v_v_tex, uv), 0.0, 1.0).r; 
	
	float spd = fit(length(vec2(vel_x, vel_y)), 0.0, 2.5, 0.0, 1.0); 
	
	vec3 vel_f = vec3(vel_x, vel_y, 0.0);
	vec3 vel_col = mix(vec3(0.1, 0.1, 1.0), vec3(1.0, 1.0, 1.0), spd) * dens;
	
	// If (within density iso threshold shade flat... color by vel)

	//frag_color = vec4(1.0, 0, 0, 1.0);
	
	//frag_color = vec4(uv, 0, 1.0); 
	
	//frag_color = vec4(dens, dens, dens, 1.0); 
	
	
	//frag_color = vec4(dens * vel_x, dens * vel_y, 0, 1.0); 
	
	//frag_color = vec4(vel_x, vel_y, 0.0, 1.0); 
	
	//frag_color = vec4(spd, spd, spd, 1.0); 
	
	frag_color = vec4(vel_col, 1.0); 
	
	//frag_color = vpos;
}
