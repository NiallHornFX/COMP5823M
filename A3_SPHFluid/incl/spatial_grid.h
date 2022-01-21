// COMP5823M - A3 : Niall Horn - spatial_grid.h
#ifndef Spatial_Grid_H
#define Spatial_Grid_H

// Std Headers
#include <vector>

// Ext Headers 
#include "ext/glm/glm.hpp" // GLM

// FD
struct Particle;
class Fluid_Object;

#define INLINE __forceinline 

// Info : 2D Uniform Grid used for particle spatial accleration / neighbourhood calculation. 

class Spatial_Grid
{
public:
	Spatial_Grid(Fluid_Object *FluidData, float CellSize, float WsSize);
	~Spatial_Grid(); 

	// Grid Functions
	void gather_particles(); 
	std::vector<Particle*> get_adjcell_particles(const Particle &pt) const;

	// Index Functions
	INLINE std::size_t idx_2Dto1D(std::size_t i, std::size_t j) const;
	INLINE glm::ivec2  idx_1Dto2D(std::size_t i)                const;

private:
	// Grid Intrinsics
	float cell_size, ws_size, r_cell_dim;
	float cell_ext, cell_ext_sqr, h_cell_ext, h_cell_ext_sqr; // extent : cell size in WS. 
	std::size_t cell_count, cell_dim; 

	// Grid Cell Data
	std::vector<Particle*> *cell_pts; 
	std::vector<float>      cell_dens; 

	// Fluid Data Ptr
	Fluid_Object *fluid_data;

	friend class Viewer; 
	friend class Fluid_Solver; 
};

#endif 