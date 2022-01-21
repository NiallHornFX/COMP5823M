// Implements
#include "ground.h"

Ground::Ground()
	: Mesh("Ground_Plane", "../../assets/mesh/grid.obj")
{
	size = 1.f, tile = 2.f; 
	load_obj(true);
	set_shader("../../shaders/ground.vert", "../../shaders/ground.frag");
	load_texture("../../assets/texture/grid_RGB.png", 0);
	tex->set_params(Texture::filter_type::LINEAR);
	set_colour(glm::vec3(1.f, 0.f, 0.f));
	mode = Render_Mode::RENDER_MESH;
}

void Ground::set_size(float Size)
{
	size = Size; 
	for (vert &v : obj_data.verts)
	{
		v.pos *= size; 
	}
	// Reset Mesh with scaled pos
	set_data_mesh(obj_data.verts);
}

void Ground::set_tile(float Tile)
{
	tile = Tile;
	for (vert &v : obj_data.verts)
	{
		v.uv *= tile;
	}
	// Reset Mesh with scaled pos
	set_data_mesh(obj_data.verts);
}