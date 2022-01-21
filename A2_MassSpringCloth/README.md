#### COMP5823M : Assignment 2 - Mass Spring Cloth

###### Niall Horn - December 2021 

___

##### Info

This is a Cloth Solver based on the Mass Spring Damper model. The program has a user interactable GUI and has the following key features : 

* Semi-Implicit Euler Integration
* Hybrid Timestepping
* OBJ Mesh I/O
* Pin Constraints
* Ground Collisions
* Sphere Collisions
* Wind and Gravity Forces

These features should satisfy the vast majority of the assignment apart from the rotating sphere simulation scenario which I ran out of time for and no feature was implemented to export the video output, as this was low priority. I will breifly outline key features 

##### Spring And Damper Forces

The ***Cloth_State*** Class stores an array of springs which are built based on the triangle edges of the input mesh, as we assume the input mesh is triangulated we get a "Shear Spring" across the face of the quadrilateral defined by 4 particles / vertices . Note we assume Vertices and Particles are the same and their indices map $1:1$ , for rendering we use index buffers to draw each triangle based on these particles / vertices. 

The Spring Force is defined as : 
$$
F_s = -K_s (||p_1 - p_0|| - rest) \: (p_1-p_0 / ||p_1 - p_0||)
$$
Where $rest$ is the rest length / original edge length of the spring in its rest / initial configuration. 

The Damper force is defined as : 
$$
F_d = [-K_d * (v_1 - v_0) \cdot  (p_1-p_0 / ||p_1 - p_0||)]\: (p_1-p_0 / ||p_1 - p_0||)
$$
Based on the order used to define the spring between the two particles $p_1 - p_0$ we then add equal and opposite forces on each particle (negative for $p_0$ and positive for $p_1$) to allow the spring to try and get back to its rest configuration. 

##### Integration : Semi Implicit Euler : 

Semi Implicit Euler integration is used, it provides slightly higher accuracy than standard Explicit Euler as it uses the newly computed $\dot{x}_{n+1}$ velocity to then calculate the updated position $x_{n+1}$ also in my tests with Runge Kutta 2 and Leapfrog it provided as good results for a lower computational cost. 

Semi Implicit Euler can be denoted as : 
$$
\dot{x}_{n+1} = \dot{x}_n + \ddot{x}_{n+1} \: \Delta t \\
x_{n+1} = x{_n} + \dot{x}_{n+1} \: \Delta t
$$


At high stiffness coefficients on the springs this can become unstable, thus Air Viscosity force is used which applies a force proportional to the negated scaled velocity : 
$$
F_{visc} = -K_v \: \dot{x}
$$
While higher order Explicit Integrators could be used, a much stabler approach would be to use Implicit Euler like David Baraff did in his paper [Large Steps in Cloth Simulation, Baraff.D et al. SIGGRAPH 1998] where he used a modified Pre-Conditioned Conjugate Gradient to solve the resulting linear system, however for real-time application this is still quite expensive and hard to multithread on modern hardware. Hence why approaches like Position Based Dynamics seem to be the most common in games and real-time applications for the stability they offer, as they do not rely on a high order integrators as most of their constraints satisfaction is done via "Projection" operations. 

##### Hybrid Timestepping : 

Not to be confused with Adaptive Timestepping which this is not ! Instead of relying on the Viewer application's timestep (which is based on the Delta Time ($\Delta t$) it takes to render the previous frame,  handle user inputs and solve the last frames simulation) which in doing so will lead to an irregular and possibly too large $\Delta t$ between timesteps which will reduce the stability of the integration especially at high stiffness coefficient forces. The other option is to used a fixed physics timestep completely decoupled from the viewer application, but this alone is no good as it means the solver timestep is decoupled from the application, so if the viewer application takes longer on one frame than another the simulation of the cloth does not account for this. Hence a Hybrid Timestepping approach is used based on the article [Fix Your Timestep, Fiedler.G, 2004] which is very popular, where we use the accumulated the delta time of the viewer application, it then subdivides this into sub-steps per frame/tick() of the solver.

 This allows for a fixed physics timestep to be used internally, while also stabile coupling the timestep to the viewer application. Note in this case the Viewer Application is my application that we use to view the simulation, in a game engine, this would be the Game Thread (which would be separate from the Physics thread of course). Based on the time the previous viewer frame took to complete the next Cloth_Solver::tick() may be divided into $0-n$ substeps to resolve the delta of the viewer application's delta time. 

The Physics timestep can be changed at anytime by the user via the GUI, by default its $\Delta t = 1/90$. 

The logic of this approach can be seen within ***Cloth_Solver.cpp*** : 

```C++
void Cloth_Solver::tick(float viewer_Dt)
{
	if (!simulate || !clothData.built_state) return; 

	// Reset Timestep (solvestep) counter (per tick)
	timestep = 0; 

	// Subdivide Accumulated Viewer Timestep into Solver Substeps
	at += viewer_Dt;
	while (at > dt)
	{
		step(); // Single Timestep
		at -= dt;
		timestep++;
	}
	frame++;
}
```

##### OBJ Mesh Import / Export : 

As shown below 3 different cloth meshes are provided that are 2D square sheets of cloth with different sizes and resolutions, these can be hot-swapped within the application GUI, which will rebuild the Cloth State (including the springs) based on the new mesh. For exporting a single frame of the cloth simulation, the export mesh path can be specified to which the simulation will then pause and write out the current cloth state to an OBJ file, this can then be loaded into any DCC like Blender to be viewed. 

The input cloth mesh can be moved to define its rest position by using the GUI "Rest_Offset" control. When ever the cloth state is reset from here the cloth will revert back to its rest position (in local space) + this offset vector. 

Switching between Cloth Meshes may take a couple of seconds as the Springs are re-built as we are using Indexed Face data the Spring building function takes a little while to compute and is around $O(n^3)$ in complexity which is not ideal and would be father optimized in the future, but only needs to be called once, for any new cloth mesh. 

Springs are built by each vertex / particle finding the triangles it is part of and connecting springs to the edges of those triangles. 

##### Collisions : 

I have implemented both Plane and Sphere Collisions, these are calculated based on the Particle positions and we assume they have a very small radius of size $\epsilon$. Like Position Based Dynamics we use the inequality constraint condtions to check if the particle is intersecting and if so we directly project the postion out by the negated signed distance. Ideally we would also correct the velocity term post projection but this sometimes leads to instability so we omit this for now. 

###### Plane Collision : 

For the case of Collision with a plane we want to satisfy the inequality : 
$$
(p-q) \cdot n \geq 0
$$
However as the plane is specified to lie at world origin $(0,0,0)$ we can omit $q$  and simply compute : 
$$
p \cdot n \geq 0
$$
This is implemented as :

```C++
float dist = glm::dot(curPt.P, N);
if (dist <= 1e-03f) curPt.P += -dist * N;
```

###### Sphere Collision : 

For Sphere Collisions we want to satisfy the inequality : 
$$
||p-c|| - r \geq 0
$$
Where $c$ is the centre of the sphere. This is implemented in code as : 

```C++
glm::vec3 vec = curPt.P - centre;
float dist = glm::length(vec);
if (dist < (radius + collision_epsilon))
{
    float inter_dist = (radius + collision_epsilon) - dist;
    curPt.P += inter_dist * glm::normalize(vec);
}
```

Not we have a small $\epsilon$ value here which can be adjusted by the GUI, to account for the fact the particles are points and have no / small radii. For meshes that have higher resolution and  thus smaller distances between vertices this $\epsilon$ value can be reduced. Ideally we would treat particles as having their own explicitly defined radii. 

The Spheres Radius and Centre can be changed via the GUI controls. Currently there is a small bug when both are changed together, but this shouldn't affect any simulation scenarios purposed by the assignment. 

These colliders can be turned on or off by the GUI also, they can even be turned on and off during a simulation (without re-setting) although doing so as the cloth first collides may result in explosions from the integration. 

##### Collision Friction : 

Collision Friction on the Cloth_Collider_Sphere class is implemented by decomposing the Velocity components to Normal and Tangential components and then using the user specified friction coefficient to define the resulting tangential component scale (which in turn defines the friction amount). 

Decomposing the Velocity into Normal and Tangential Components (where we use the direction of the Particle to the Sphere) is done simply via : 

```C++
glm::vec3 N = glm::normalize(vec);
glm::vec3 v_N = glm::dot(curPt.V, N) * N;
glm::vec3 v_T = curPt.V - v_N;
```

For the Collision Code, please see the file ***Cloth_Collider.cpp***

##### External Forces + Lack of Self Collisions. 

Gravity and Wind can be controlled by the GUI, due to the lack of self collisions, if forces are too high visible self intersection may become apparent as it may for collisions with the ground plane. 

However this was not part of the assignment and while I have implemented self collisions before for cloth, I did not have time in this case, also resolving self Collisions for Mass-Spring based cloth solvers is quite hard to do well, PBD approaches are easier as positions constrained can be iteratively sat sifted in a stable manor without relying on forces or impulse to prevent self collisions. 

____

##### Key Classes / Files

As this project is made up of the core "Viewer Application" which is the GUI, Input and OpenGL side of the application used for rendering and interacting with the Cloth and the Cloth Simulation code itself. To make marking easier here is a small breakdown of the key files with relation to the Cloth Simulation code : 

* ***Cloth_State.h / .cpp*** : Defines the state of the cloth (Particles, Springs) functions for loading and exporting the cloth mesh. It is responsbile for building the springs between the particles. 
* ***Cloth_Mesh.h / .cpp*** : This is a derived class from Primitive which is my base class for rendering any mesh in the application. It implementes a custom rendering path using index buffers (which makes sense for cloth as we have shared particles / vertices among multiple triangle faces) it also adds support for rendering the particles as points and the edges as lines. A key function is the calc_normals() function which updates the normals per frame / tick. Also assuming the input mesh is a 2D Grid it also caluclates UV Coordinates. 
* ***Cloth_Solver.h / .cpp*** : This is where the simulation is calucated and updated onto the Cloth_State data, we evalulate the cloth springs, accumulate the forces, evaulate the colliders and then peform integration.
* ***Cloth_Collider.h/.cpp*** : This is a set of basic classes which implement a common virtual member function "Cloth_Collider::eval_collision()" which takes in the Cloth_State particle array and solves the Collision Inequality equation projecting the particles if they violate / intersect the collider primtivie. The Cloth_Collider_Sphere class also has a Mesh Primitive which is used to render it within the Viewer Application.

The Cloth State including the Solver and Colliders are allocated within ***Viewer.cpp*** and then passed to their respective objects. All of the GUI state is also set within this source file. 

##### Viewer Application

The rest of the source files make up the GUI Viewer Application which is based on using Modern OpenGL (4.0+) to render the cloth, grid and collider meshes in the scene. This is the same base I used for the IK assignment, but has been expanded further eg to add Blinn-Phong lighting for the cloth.  Please make sure that the shaders are left unchanged and within their orginal directory : 

`/shaders` these will be compiled once the application launches.

Their may be a small delay from launching the application to the first frame, this time is used to Compile the Shaders and build the springs of the Cloth Mesh. 

___

##### Usage

Launch the application from  `/build/bin/COMP5823M_A2.exe` and use the GUI to control the Cloth. 

Moving through space can be done using the `W,A,S,D,Q,E` keys, `W,A,S,D` strafe and move the camera, holding `Middle Mouse Button (MMB)` allows the camera to be panned, `Q,E` allows the camera to be raised or dropped along the relative Y Axis. 

3 Cloth Meshes are included within `/assets/mesh` these can be changed by changing the file path within the GUI "Mesh Import" field to one of the follow : 

* ***"../../assets/mesh/clothgrid_a.obj"*** : 2x2 Meter sheet of cloth with 16^2 Particles.
* ***"../../assets/mesh/clothgrid_b.obj"*** : 4x4 Meter sheet of cloth with 16^2 Particles.
* ***"../../assets/mesh/clothgrid_c.obj"*** : 4x4 Meter sheet of cloth with 24^2 Particles.

____

##### Building

This project was developed on Windows using MSVC C++ (vc141, Visual Studio 2017 (15.9.19)) with the C++ 14 Language Standard. It has not been tested on Linux. 

Make sure the working directory is `/build/bin`/ because the file paths are relative to this directory, for example shaders are loaded via `../../shaders/` directory path. Place `.obj` cloth files into `/assets/mesh/`.

Make sure `glew32.dll` and `glfw3.dll` are copied into the `/build/bin/` directory for runtime linking. They should be in this directory by default. 

___

##### Dependencies

* **GLM** - For core linear algebra / transformation operations
* **GLEW** - OpenGL Function loading. *Ideally eliminate this dependency if time*. 
* **GLFW** - OpenGL Context, Window and Input handling. 
* **DearImGui** - GUI (via GLFW and OpenGL Implementation backends).
* **Stb Image** - Texture image file loading (jpg, png, tiff, etc). 

You will of course need to link with OpenGL `opengl32.lib` on Windows (which is enabled in the project file by default).

____

##### References 

 [Fix Your Timestep, Fiedler.G, 2004] : https://gafferongames.com/post/fix_your_timestep/

 [Large Steps in Cloth Simulation, Baraff.D et al. SIGGRAPH 1998]  : https://www.cs.cmu.edu/~baraff/papers/sig98.pdf
