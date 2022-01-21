#### COMP5823M : Assignment 2 - SPH Fluid

###### Niall Horn - December 2021 

___

##### Info

This project is a 2D Smoothed Particle Hydrodynamics (SPH) Fluid Solver primarily based off of the paper ***[Particle-Based Fluid Simulation for Interactive Applications, Muller et al, SCA 2003]***. It implements the following features : 

* Core Application with GUI control for all key parameters

* Poly6, Spiky and Viscosity smoothing kernel functions 
* Spatial acceleration for particle neighbourhood computations
* Leapfrog Symplectic Time Integration
* Fluid Viscosity and Surface Tension internal forces. 
* External forces (Gravity and Air Resistance)
* Collisions with "fluid tank" boundaries. 
* Adjustable fluid configuration (Position, Size, Spacing, Jitter)
* Rendering - as Points 
* Rendering - as Surface (Metaball based)

**Below I highlight a few implementation details :** 

##### Fluid Object vs Fluid Solver :

As with my previous assignments I follow a object oriented approach where the simulation operations and state are separated from one another. While both classes have bidirectional "pipes" / access between them it makes more sense to keep the fluid data separate from the solver itself. The `Fluid_Object` defines the state of the simulation and the data along with the min and max values of each fluid quantity which is written to by the `Fluid_Solver` for each simulation step. `Fluid_Object` also implements the functionality to render the resulting fluid data using methods described below. 

##### Fluid Configuration : 

The configuration of the fluid is parametrized by the application GUI, this allows for adjusting the fluid position within the $10m^2$ simulation domain, the size of the fluid square to be emitted, the spacing (resolution) of the fluid particles along with an option to jitter the initial positions to break up the initial raster grid pattern which can ensure a more natural distribution of the fluid particles is achieved. These parameters define the state to construct new `Fluid_Object` instances when modified by user. 

##### Smoothing Kernels : 

As per Muller's paper the Poly6 kernel is used for density and colour field computation, however due to its vanishing derivative towards the bounds of the smoothing radius we use the Debrun's Spiky kernel to compute the pressure gradient, furthermore due to similar issues with with the second derivative we use a separate viscosity smoothing kernel to calculate the Laplacian for the viscosity.

The Kernel function and their functions to be used for spatial derivative computations are : 

###### Poly6 : 

$$
\omega_{poly6} (r, h) = {315\over64\pi h^9} \left\{\begin{array}{} (h^2-r^2)^3 \:\:\:\: 0 \leq r \leq h\\ \:\:\:\:\:\:\:\: 0 \:\:\:\:\:\:\:\:\:\:\:\: otherwise \end{array}\right.
$$

$$
\nabla\omega_{poly6} (r, h) = -{945\over 32\pi h^9} {\vec{r}\over ||\vec{r}||} (h-|r|^2)^2
$$

###### Spiky : 

$$
\omega_{spiky} (r, h) = {15\over\pi h^6} \left\{\begin{array}{} (h-r)^3 \:\:\:\: 0 \leq r \leq h\\ \:\:\:\:\:\:\:\: 0 \:\:\:\:\:\:\:\:\:\:\:\: otherwise \end{array}\right.
$$

$$
\nabla\omega_{spiky} (r, h) = -({45\over\pi h^6} (h-||r||)^2) {\vec{r}\over ||r||}
$$

###### Viscosity : 

$$
\omega_{visc} (r, h) = {15\over2\pi h^3} \left\{\begin{array}{} -{||r||^3 \over 2h^3} + {||r||^2 \over h^2} + {h \over2||r||} - 1\:\:\:\: 0 \leq r \leq h\\ \:\:\:\:\:\:\:\: 0 \:\:\:\:\:\:\:\:\:\:\:\: otherwise \end{array}\right.
$$

$$
\nabla^2\omega_{visc}(r,h) = {45\over \pi h^5}\cdot (h-||r||)
$$

*Note : Viscosity Kernel and Gradient is not used, only the second derivative for Laplacian calculations.*

Functions within the `Fluid_Solver` class are written so that the alternate kernels can be easily swapped out by passing them into the attribute computation operations as function pointers. This is needed to fullfill the part of the assignment where we are asked to first compute the pressure gradient using Poly6 kernel based gradient function and then replace with the Spiky kernel gradient function. The function pointer typedefs are defined within `Fluid_Solver.h` :

```C++
// Fluid_Solver.h
using kernel_func      = float(Fluid_Solver::*) (const glm::vec3 &r);
using kernel_grad_func = glm::vec2(Fluid_Solver::*)(const glm::vec3 &r);
using kernel_lapl_func = float(Fluid_Solver::*) (const glm::vec3 &r);
```

I specifically designed the `Fluid_Solver` class so that Smoothing Length / Kernel Radius $h$ is fixed and passed on solver construction to allow for precomputation of the scalar coefficients of the kernel functions within the construction : 

```C++
// Fluid_Solver.cpp : Fluid_Solver::Fluid_Solver(..)
// Poly 6
poly6_s = 4.f / M_PI  * std::powf(kernel_radius, 8.f);
poly6_grad_s = -(24.f / M_PI * std::powf(kernel_radius, 8.f));
// Spiky
spiky_s = 10.f / M_PI * std::powf(kernel_radius, 5.f);
spiky_grad_s = -(30.f / M_PI * std::powf(kernel_radius, 5.f));
// Viscosity
visc_lapl_s = -(20.f / M_PI * std::powf(kernel_radius, 5.f));
```

This avoids re-calculating them every time an operation using the kernel is invoked (which as you imagine is a lot). However it means that when the Kernel Radius is changed by the user, the Fluid_Solver has to be re-allocated, however this is not too costly given that it's never done when a simulation is in progress. 

##### Spatial Acceleration : 

Due to the need to access all neighbouring particles within the Kernel Radius $h$ we want to eliminate the typical $O(n^2)$ time complexity of each particle searching every other particle. To do this we use a Spatial Acceleration data structure. 

Initially I implemented spatial acceleration using a 2D spatial hash grid based on the paper ***[Optimized Spatial Hashing for Collision Detection of Deformable Objects, Teschner et al, 2003]***, however due to the lack of spatial locality in the resulting hash grid and the more expensive operations needed to get adjacent cells of each particle I switched to a standard 2D Uniform Grid. This is implemented within the `Spatial_Grid` class. 

Within `Fluid_Solver::get_neighbours()` a new Spatial_Grid instance is allocated for the current timestep, each cell of the grid is transformed into the simulation domain space and checks for particles that lie within its cell bounds within the `Spatial_Grid::gather_particles()` function this is of $O(n)$ time complexity with $n$ been the number of particles. Each particle then gets and stores a concatenated array of its neighbouring particles, these are particles that lie within the same grid cell as itself plus its 8 neighbours, this is computed within `Spatial_Grid::get_adjcell_particles()`. This is then cached into a per particle array within `Fluid_Object`. 

The Grid cell size is fixed to the Kernel Radius $h$ to ensure that no matter the particle location within the cell, the particle neighbours are always within the smoothing radius.

##### Leapfrog Integration : 

As per the assignment brief for time integration the Leapfrog method is used, this evaluates the particle forces twice to first compute $x_{n+1}$ before then computing $\dot{x}_{n+1}$. Internal particle forces are scaled by the particle densities with external forces gravity and air resistance also added. With the method been a Symplectic Integrator it provides more stable time integration than standard explicit schemes such as Runge Kutta while still been quite cheap to compute. As a unit test I also implemented the Semi-Implicit Euler method to compare with, however this is disabled by default. 

The Leapfrog method is defined as : 
$$
x_{n+1} = x_n + \dot{x}_n \Delta t + {1\over2}\ddot{x}_n \Delta t^2 \\
\dot{x}_{n+1} = \dot{x}_n + {1\over 2} (\ddot{x_n}+ \ddot{x}_{n+1})\Delta t
$$

##### Internal Particle Forces : 

The core of any SPH solver is the computation of particle forces, following Muller's paper we first calculate the density of the fluid at each particle location $p_i$ using the Poly6 Kernel. This is then used to compute the pressure using the "Equation of State" (EOS) purposed in the paper :
$$
p_i = k(\rho_i - \rho_0)
$$
 With $k$ been the Gas Constant / Pressure stiffness and $\rho_0$ been the rest density of the fluid. My solver has an option to compute the rest density $\rho_0$ by computing the max density of the fluid on frame 0 scaled by some small $\epsilon$ offset. This saves the need for the user to guess the rest density which can be tedious at times however this can be turned off. When the density of the fluid exceeds the rest density pressure is created which should force the particles away from each other, to maintain the rest density along the negative pressure gradient. 

The pressure gradient is computed within `Fluid_Solver::eval_forces()`. Due to the need for forces to be evaluated twice within a single simulation timestep, eval forces does not loop over all particles, but instead takes in a particle as a parameter so it can be called within the per particle integration loop. The function consist for the computation of 3 separate forces :

* Pressure : By computing the negative pressure gradient of particle neighbours.
* Viscosity : By computing the Laplacian of the velocity difference of particle neighbours. 
* Surface Tension : By computing the Gradient and Laplacian of the Colour Field. 

###### Pressure : 

Pressure described above uses the Spiky Kernel gradient to compute the resulting negative pressure gradient as described by Muller et al using a symmetric formulation : 
$$
\textbf{f}_i^{pres} = -\nabla p(r_i) = -\sum_j m_j {{p_i + p_j}\over 2\rho_j}\nabla\omega(r-r_j, h)
$$
Note that for neighbourhood calculation of pressure we ensure $pt_i \neq pt_j$ thus we skip the particle self within its own neighbours, otherwise this will cause numerical issues as the resulting $r$ vector would be 0. 

Pressure is clamped to be positive only, this ensure that's the pressure is only used to maintain the volume of the fluid and approximate incompressibility. For pressure negative terms we will use the surface tension force which can be parameterized and thus controlled separately. 

###### Viscosity : 

The Viscosity force is the calculated using the velocity differences of neighbouring particles scaled by the Laplacian of the Viscosity kernel to symmetrize the resulting force :
$$
f_i^{visc} = \mu \sum_j m_j {\textbf{v}_j - \textbf{v}_i \over \rho_j} \nabla^2\omega(r-r_j, h)
$$
This is scaled my $\mu$ the kinematic viscosity coefficient as defined in the Navier Stokes momentum equation. 

 ###### Surface Tension : 

For surface tension we follow the Muller et al approach by using the concept of a "Colour Field" which defines the two phases of fluid in this case water and air, which when differentiated to define the direction to the fluid surface. Surface tension should minimize the curvature of the fluid by attracting surrounding fluid. While the lecture slides suggest using a grid for this calculation, we keep our calculations on the particles by following Muller et al and calculating the smoothed colour field on the particles using the Poly6 Kernel : 
$$
c(r) = \sum_j m_j {1\over \rho_j}\omega(r-r_j, h)
$$
When then differentiate the resulting Colour Field values per particle using the purposed methods for computation of Gradient and Laplacian above to define the resulting surface tension force as : 
$$
f_i^{surf} = -\sigma \nabla^2c {\nabla c \over |\nabla c|}
$$
Where $-\sigma$ is the tension coefficient, $\nabla^2c$ defines the Laplacian of the colour field which is an approximation of the curvature, multiplying these scalar terms by the normalized Colour Field gradient $\nabla c$ we define the surface tension force. As per the paper, we first check that the magnitude of the gradient is non zero before doing this to prevent numerical errors. 

##### Collisions

Collisions use a similar polymorphic class to my previous assignment where a virtual member function evaluates the collision detection and response on to the array of particles defining the fluid. 

Collisions with the fluid tank boundaries are defined as 3 plane collisions, with there normals facing inwards towards the tank centre. For the purpose of this solver ghost particles are not used to line the boundaries but instead we use a more ad-hoc solution as simply project the particles back to the surface of each side of the fluid tank when the collision is violated. This is to ensure the satisfaction of the inequality constraint : 
$$
(p-q) \cdot n \geq 0
$$
To approximate transfer of momentum from collision we then decompose the particles velocity into Normal and Tangential components with coefficient's been used to scale the resulting components. We reduce the normal components to a greater extent to impose free-slip like boundary conditions where the fluid can slip while still colliding along the negative direction of the normal to some extent. 

Decomposition of the velocity components is implemented in the following Lambda Function : 

```C++
// Fluid_Collider.cpp
// Fluid_Collider_Plane::eval_collision()
auto vel_decomp = [this](const glm::vec3 &v, float tang_mult, float norm_mult) -> glm::vec3
{
	// Decompose to tangential and normal components
	glm::vec3 v_N = glm::dot(v, N) * N;
	glm::vec3 v_T = v - v_N;
	return (v_T * tang_mult) + ((-v_N) * norm_mult);
};
```

##### Simulation Step : 

A single simulation step consists of the following operations : 

```C++
// Build Acceleration Grid and cache particle neighbours
get_neighbours();
// Compute Density and from this pressure using some smoothing kernel.
compute_dens_pres(&Fluid_Solver::kernel_poly6);
// Compute Colour Field 
calc_colour_field();
// Check for collisions and project offending particles. 
eval_colliders();
// Calculcate resulting forces and integrate
integrate();
```

##### Rendering :

I implemented two "Render Paths" for displaying the fluid into the application. Both of these can be enabled simultaneously.  Rendering is invoked at the end of each `Viewer::tick()` call via the `Fluid_Object::render()` function, drawing the results of the latest simulation data. 

###### Point Based Rendering : 

For Point Based Rendering, the fluid particles are passed to a `Primtive` class which is a core class of my viewer application which defines a basic mesh object for rendering via OpenGL. The particles are passed as vertices of a mesh and drawn as `GL_Points`. There is a number of particle / point colour options which are handy for debugging and visualizing the fluid quantities on the particles. These include : 

* Velocity
* Pressure
* Density
* Grid Cell

The Grid cell option been useful for visually seeing the size of the grid cells from the `Spatial_Grid` class used to accelerate smoothing kernel computation over neighbouring particles. 

The size of the points is based on the spacing multiplied by a user controlled radius parameter in the GUI. 

###### Surface Based Rendering (Metaballs) : 

Surface based rendering is implemented using the Wyvill Metaball Algorithm ***[Data Structure for Soft Objects, Wyvill et al, 1986]***:
$$
f_i(r_i) = \left\{\begin{array}{} 1-3(r_i/R)^2 + 3(r_i/R)^4 - (r_i/R)^6 \:\:\: r_i \leq R \\  0 \:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:\:
r_i > R \end{array}\right.
$$
Where $r_i$ is the distance of the current position to the field centre and $R$ is the desired radius. 

However oppose to using marching squares to then extract a mesh from the resulting field values, we compute them directly per fragment. Where particles are passed to the GPU using OpenGL's Shader Storage Buffer Object's. This requires OpenGL 4.3 compatibility (which is assumed on all platforms apart from Mac OS where only OpenGL 4.1 core is supported, however this project is for Windows target and untested on Mac OS).

Particle data is truncated from the CPU Particle struct to the GPU version where only Position, Velocity and Density is passed : 

```C++
// Fluid_Object.h
struct Particle
{
	Particle(const glm::vec3 &p, float Mass, std::size_t idx)
		: P(p), mass(Mass), rest(P), V(glm::vec3(0.f)), id(idx), cell_idx(-1),
		   cf(0.f), density(0.f), pressure(0.f) {}

	glm::vec3 P, V;
	glm::vec3 rest; 
	std::size_t id, cell_idx;
	float density, pressure, mass, cf;
};

struct alignas(16) Particle_GPU
{
	glm::vec2 pos;
	glm::vec2 vel; 
	float dens; 
};
```

 Due to the layout alignment and padding of the data on the GPU we enforce 16 byte boundary alignment of the `Particle_GPU` struct. 

The shader program for the Metaball render path then rasterizes a full screen quad which is mapped to the simulation domain space, each fragment loops over particles within the bound SSBO and accumulates the meatball field function shown above, this is implemented into GLSL as : 

```GLSL
// fluid_quad.frag
float meta(vec2 r, float h) 
{
	float rl = length(r); 
	if (rl > h) return 0.0;
	float rlh = rl / h;
	return 1.0 - 3.0 * pow(rlh, 2.0) + 3.0 * pow(rlh, 4.0) - pow(rlh, 6.0);
}
```

We then draw the resulting Metaball isosurface defined by user specified Iso-threshold and radius parameters accessible in the GUI. We also accumulate each fragments total speed and this defines the colour of the resulting rasterized surface. 

**Note:** At high particle counts, this will bottleneck the application, due to the number of per fragment operations needed looping over all particles. Ideally these would be pre-culled spatially relative to each fragment to avoid looping over all particles or first rasterized onto a lower resolution grid that is then sampled as a texture within the fragment shader. 


____

##### Key Classes / Files

As this project is made up of the core "Viewer Application" which define the GUI, Input and OpenGL side of the application used for rendering and interacting with the Fluid and the Fluid code itself. The Fluid Object and Solver are allocated within ***Viewer.cpp***.  All of the GUI state is also set within this source file using the DearImGUI library. 

To make marking easier here is a small breakdown of the key files with relation to the Fluid Simulation code: 

* ***Fluid_Object.h / .cpp*** : Class that contains the Fluid state and data, including the particle array and per timestep cached neighbour lists of particles. This class also implements the functionality for rendering the fluid as ethier points or a surface. 
* ***Fluid_Solver.h / .cpp*** : Contains the core solver functionality used for the fluid simulation. Both Fluid_Object and Fluid_Solver have bi-directional access through pointers with friend level access to each others data. 
* ***Fluid_Collider.h / .cpp*** : Contains a basic polymorphic class which is used to implement a plane collision primitive used to calculate collision detection and response between the fluid and the sides of the tank, along with the boundaries of the simulation domain. 
* ***Spatial_Grid.h/.cpp*** : Class that defines a Uniform 2D Grid over the simulation domain which is used to accelerate the fluid quaintly calculations using kernels, by gathering particles into grid cells and returning per particle neighbourhood lists based on self and adjacent grid cells. 

##### Viewer Application

The rest of the source files make up the GUI Viewer Application which is based on using Modern OpenGL (4.3+) to render the Fluid, Colliders and GUI. This is based on the same classes i've used for the previous assignments but modified to render a 2D Scene and removed most 3D calculations. Note that some calculations still use 3D Vectors but these are truncated to 2D where needed.  Please make sure that the shaders are left unchanged and within their original directory : 

##### Shaders : 

`/shaders` these will be compiled once the application launches : 

* ***fluid_points.vert / .frag*** : Defines the shader program for rendering the fluid particles as vertices / points. 
* ***fluid_quad.vert / .frag*** : Defines the shader program that is used for rendering a full screen quad to rasterize the particle metaballs on using the bound SSBO containing the particle simulation data. 
* ***collider.vert / .frag*** : Defines the shader program used for rendering the colliders. 

___

##### Usage

Launch the application from  `/build/bin/COMP5823M_A3.exe` and use the GUI to control the fluid. 

The initial configuration of the fluid follows the specification defined in the assignment where the fluid is a square "blob" of dimensions $2m \times 3m$ at $height = 4m$. For the part of  the assignment where we are asked to use the Poly6 Gradient for pressure computation (which causes various instabilities as defined by Muller et al, hence the use of Spiky) there is an option to specify which kernel is used for the pressure gradient calculation. Default is Spiky however it can be switched to Poly6, the GUI shows the current kernel used for pressure. 

The Spacing slider can be used to increase / decrease the fluid resolution. Kernel Radius is not coupled to this however as per Muller et al its recommended that the radius is around 2 times the fluid spacing. In my tests i've found this to be ok in some situations but not in others, hence I left it as a free parameter. The default kernel radius is $h = 0.5$. 

Looking at the `Fluid Attributes` section of the GUI you will see the min and max values of various key fluid quantities in the simulation this is useful for debugging sake, these values also drive the min and max ranges used for scaling the values for visualization colour ranges. Note that if `Compute Rest Dens` is enabled (which it is by default) the rest density will be computed once the simulation has been reset and the next simulation has been started. So adjusting slider values then resetting the simulation ensures the new rest density is calculated. 

Viscosity and Surface Tension are disabled by default, they can be enabled and their coefficients set in the GUI options. By default gravity is set to $g = -10$ and Air Resistance $\omega = 0.5$, these can be adjusted at anytime even when the simulation is running. The default pressure Stiffness / Gas Constant $k = 500$ values above $k = 1000$ are typically not needed in the current simulation domain scale. 

The default rendering options are for both the Surface and particles, note that the surface is only generated on the first frame of simulation while the particles are persistent even when simulation is disabled. As per above, the point based rendering mode has options for colour visualizations of the various particle attributes as well as visualizing the bounds of the cells of the Spatial Acceleration Grid. 

____

##### Building

This project was developed on Windows using MSVC C++ (vc141, Visual Studio 2017 (15.9.19)) with the C++ 14 Language Standard. It has not been tested on Linux. Building in Debug configuration will result in significant performance compromises. 

OpenGL 4.3 is required due to the use of Shader Storage Buffer Objects for passing particle data from the host to the GPU for evaluation within the fragment shaders. All modern GPU drivers should implement OpenGL 4.6, I don't see this been a problem in 2022 ! 

Make sure the working directory is `/build/bin`/ because the file paths are relative to this directory, for example shaders are loaded via `../../shaders/` directory path. 

Make sure `glew32.dll` and `glfw3.dll` are copied into the `/build/bin/` directory for runtime linking. They should be in this directory by default. 

___

##### Dependencies

* **GLM**   - For core linear algebra / transformation operations
* **GLEW** - OpenGL Function loading.
* **GLFW** - OpenGL Context, Window and Input handling. 
* **DearImGui** - GUI (via GLFW and OpenGL Implementation backends).

You will of course need to link with OpenGL `opengl32.lib` on Windows (which is enabled in the project file by default).

____

##### References 

[Particle-Based Fluid Simulation for Interactive Applications, Muller et al, SCA 2003] : https://matthias-research.github.io/pages/publications/sca03.pdf

[Optimized Spatial Hashing for Collision Detection of Deformable Objects, Teschner et al, 2003] : https://matthias-research.github.io/pages/publications/tetraederCollision.pdf

[Data Structure for Soft Objects, Wyvill et al, 1986] : http://webhome.cs.uvic.ca/~blob/publications/datastructures.pdf
