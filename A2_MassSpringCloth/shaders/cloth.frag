// COMP5823M - A2 : Niall Horn - cloth.frag
#version 400 core 

// Input
in vec3 colour;
in vec3 normal;
in vec3 uvw; 
in vec3 pos_world;
in vec3 normal_world; 

// Uniforms 
uniform vec3 camPos_world; 
uniform vec3 lightPos_world;
uniform float lightStr; 
uniform bool ren_normals; 

// Output 
out vec4 frag_colour; 

vec3 checker(in float u, in float v, in float size)
{
  float fmodResult = mod(floor(size * u) + floor(size * v), 2.0);
  float res = max(sign(fmodResult), 0.0);
  return vec3(res, res, res);
}

void main()
{
	// Normals or Checker ? 
	vec3 r_colour = mix((checker(uvw.x, uvw.y, 10) * 0.25) + 0.2, vec3(0.1, 0.8, 0.1), 0.4); 

	// Blinn-Phong (WS)
	vec3 light_dir    = normalize(lightPos_world - pos_world); 
	vec3 view_dir     = normalize(camPos_world - pos_world);
	vec3 half_v       = normalize(light_dir + view_dir);
	vec3 a = 0.125 * r_colour; 
	vec3 d = (max(dot(normal_world, light_dir), 0.0) * r_colour) * lightStr; 
	vec3 s = (pow(max(dot(normal_world, half_v), 0.0), 8.0) * vec3(0.9,0.9,0.9)) * lightStr; 
	
	if (ren_normals)
	{
		frag_colour = vec4(normal, 1.0); 
	}
	else
	{
		frag_colour = vec4(a+d+s, 1.0); 
	}
	
}
