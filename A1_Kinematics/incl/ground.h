#ifndef GROUND_H
#define GROUND_H

// Project Headers
#include "mesh.h"

class Ground : public Mesh
{
public:
	Ground();
	~Ground() = default;

	void set_size(float Size);
	void set_tile(float Tile);

public:
	float size, tile; 
};

#endif