#### COMP5823M - Assignment 2 - Cloth Simulation - Notes

##### Niall Horn - University of Leeds - 2021

___

#### Initial Notes

I'm not going to have time to write many notes for this one, its mass spring based, i've done this before. However even though this is based on a 2D plane / flat piece of cloth its required to implement this via a loaded obj mesh, using edges as springs (ie only distance springs) however I may add shear and dihedral (although this is a bit ambiguous with an instructed obj mesh). 

The base of the app is my "Viewer Application" I wrote for A1 but without any of  the animation code ofcourse, this gives me a nice basis, with a GUI to add the solver into. 

Basic Collision detection with a sphere primitive (defined parametrically, rendered using a sphere mesh).

Integration will be based on Semi Implicit Euler and possibly if I have time I will add another scheme like RK2 or RK4. 

Some parts of the assignment like been able to render out a play blast to a video file is not really a big prio. 

Shading wise I will use Blinn-Phong on the cloth so we can see its shape more, along with toggle modes for the springs / edges and vertices (masses/particles). 

I wanted to be hip and add self collision based on my Verlet Cloth / PBD project and prior knowledge of writing cloth solvers, but this is only if I have time. 

Ideally we can switch between the desired simulation scenarios purposed (hanging cloth, falling cloth onto sphere, rotating sphere), surprised they asked for rotating sphere when there's no self collisions required, this will look ugly. Want to implement correct normal and tangential decomp friction...

To get the shear spring on the diagonal (tri hyp) we could load in obj meshes as quads, and then triangulate them internally so we know which edge is the shear spring ? For now I'm just using my VerletCloth Workflow where I loop over particles, tris (based on indices) and form edges from these, but their is little control / way of defining where each type of spring should be, they are assume to all just be distance / struct springs. 

As we see below I'm using the same approach as my VerletClothSolver project where each unique vert positions becomes a particle hence each vertex maps directly a a particle so particle and vertex is used synonymously but typically particle refers to the point for simulation and vertex for rendering but can be interchanged so note this. For rendering ofcourse each particle/vert is indexed via an EBO based on the per tri indices as each point is shared across multiple tris, hence my cloth_mesh class was written specifically for indexed drawing of the cloth particles with per-tick computed attributes. 

Classes made for this project : 

* Cloth_State 
* Cloth_Solver
* Cloth_Mesh
* Cloth_Collider

____

##### Cloth_State Class

This will include obj loading of the passed input mesh (via the GUI). All state update on the cloth simulation state (particles, springs) and render state (vertices & tris) will be done within here along with the final render() called from viewer app render loop. The Simulation will be done within the Cloth_Solver class which references a cloth object and is modifies its state based on the solve. The Solver will be ticked also from the viewer application loop. 

This creates a nice decoupling of cloth state + data and the solver itself which modifies this. 

Spring + Particle Struct declarations

Plan to try and optimize the mesh / primitive classes to make the cloth -> mesh data update per frame more efficient, but it seems to be ok for now. 

We use the mesh class for rendering which I wrote initially to be a primitive that loads an obj file, with texture support, however in this case we are passing it a pre-parsed set of vertices, and we need index drawing, so I may need to modify mesh or create a new class to account for this, could call it "cloth_mesh" and then implement indexed drawing (also this will not inherit from Primitive as I need to add EBO setup etc). Or we could use the mesh class and map each indexed vertex ( and thus particle) to each duplicate vertex using the mesh classes triangle soup approach and then the resulting vertices update their positions from the particle per frame while keeping their original attributes and thus we draw using non indexed method, but this is not very clean. 

obj_loading both loads the vert position data and indices (to form unique particles per vertex position, see below) but also creates the particle data. So anytime a new obj is loaded this will be re-created, but most likely for this project the obj file will be the same, although would be cool to test with some other meshes, been careful of the hardcoded limitations / assumptions of it been a square piece of cloth tri mesh for this project. 

Cloth_Mesh will be re-created for each new obj file (most likely that won't occur as the assignment only deals with a single 2D plane cloth mesh) could also define seperate cloth_meshes for difference simulation scenarios (pinned corners, freefalling etc and switch between them based on GUI, oppose to modifying state of a single instance each time).

##### render() :

Calls render on cloth_mesh but also renders the visualizer primitives for springs and particles based on if enabled in gui. 

##### buildcloth() :

* Creates the springs based on particle pair edges of particles defined indices from resulting particle data from obj loading. 
* Calculates spring  rest lengths. 

As we ideally want all 3 types of springs you typically see in a mass-spring solver we need to make sure the indices are correct so have distance springs along the quad edges, shear springs along the tri hyp edges and then bend springs skipping over each adjacent particle (typically would be in face/tri centres as dihedral springs) bend/dihedral springs are not a prio for now. 

The way I did this in VerletClothMesh was to pre-compute and store per particle (vert) tris they are a part of (remember tris are stored as vec3<int>s for each index), then from this per particle we loop over each tri and create a constraint along each edge, I also used a hash function and check to determine if two particles already had a constraint (eg for the same particle pair in the inverse direction). I could use this same approach but it makes determining where to add shear and bend springs hard, as we'd just typically treat each spring as distance, we could dot each edge against themselves to find the hyp but this is slow. 

For now we will just assume all springs are struct / distance as shear springs use the same forces anyway the spring types were mainly for identification purposes, as long as their is springs along each edge (including all tri edges, ideally without reciprocal duplicates) then that itself implicitly defines struct and shear springs (all as the same spring type). Of course the quality of these springs depends on how clean the topology is of the mesh, but for the test case purposed in the assignment this won't be an issue. 

###### Per Particle Triangle Array

To pre-compute the per particle triangles (which tri is each particle (aka vertex)) a part of I use a basic scatter approach where we loop through each tri (which is just a glm::ivec3 with 3 vert/particle indices) for each index pass a ptr to this current tri to an 2D array which is a per particle array, array of triangle ptrs whom contain its index. Of course this only needs to be computed once per new cloth mesh : 

```C++
void Cloth_State::get_particle_trilist()
{
	// Reset per particle inner array tri list. 
	pt_tris.resize(particles.size());
	for (std::vector<glm::ivec3*> trilist : pt_tris) trilist.clear();
    
    // Loop over tris 
    for (glm::ivec3 &tri : tri_inds)
    {
        // Add tri ptr to each particle defined by tri indices tri list array. 
        for (std::size_t i = 0; i < 3; ++i)
        {
            int p_ind = tri[i];
            pt_tris[p_ind].push_back(&tri);
        }
    }
}    
```

This seems to work as for a 2D plane we expect most particles will be part of 6 tris, other than particles on the edges or at the corners whom will only be part of 1-4. 

###### Building Springs

We will worry about duplicates / con hash checking in a bit, for now lets just get the spring creation working. Using similar approach to VerletClothMesh we loop over per particle tris (its part of), get each other particle at each tri index, check its not self and create spring between particle self and the other particle defined by the tri indices. 

```c++
// Cloth_State::build_cloth()
// ============= Build Cloth Springs ==============
	// For each particle
	for (std::size_t p = 0; p < particles.size(); ++p)
	{
		Particle &curPt = particles[p];

		// For each tri, particle is part of
		std::vector<glm::ivec3*> &triPts = pt_tris[p];
		for (std::size_t t = 0; t < triPts.size(); ++t)
		{
			// For each tri index (Adjacent Particles)
			for (std::size_t i = 0; i < 3; ++i)
			{
				std::size_t ind = (*triPts[t])[i];
				assert(curPt.id == p); // Make sure iter index matches Pt_id. 
				// Build Spring for particle pair
				if (curPt.id != ind) // Make sure we dont make spring with self pt
				{
					// Get other particle defined by index
					Particle &othPt = particles[ind];

					// Compute Rest Length 
					float rl = glm::length(curPt.P - othPt.P);

					// Create Spring
					springs.emplace_back(&curPt, &othPt, rl);

					// Increment Cur and Oth Particle Spring Count
					curPt.spring_count++, othPt.spring_count++;
				}
			}
		}
	}
```



###### reset_cloth() :

will reset the cloth particle positions to their initial state and removes any set forces, velocities on them. 

____

##### Cloth_State : OBJ Loading

We need to treat the mesh as a index face data structure as we need a single mass point / particle for each unique vertex only. 

For rendering sake we will keep all attribute of the input mesh (including the initial normals, which will need to be re-calculated per frame this can be done on the particles or the resulting render verts internally using adjacent verts).

As we need adjacent vertices to form edges this is where a smarter data structure like half edge or directed edge might benefit, however as my VerletCloth Solver used pure index_face vertices --> particles and constraints (springs in this case) I'm going to try and replicate that approach to get adjacent verts and thus edges to define springs along. 

Could just do normal obj loading (Creating duplicate verts) and then do a triangle wise loop (for very 3 verts) and from this, form the indices and thus particles. Problem is as verts have other attributes it makes indexing them harder as we can only use a single EBO.

I think the easiest thing to do for now is just discard all input attributes other than vert position (and thus particles will be based on vert pos indices), we will re-calc normals per tick and in this case, because its a flat 2D plane (as an obj file) we can re-calc the texture coords ourselves, therefore these are only done on the particle / unique positions vertices and can use indexed drawing, we also need to store the input obj vertex pos indices as these are what define these verts and thus the particles and thus we need to store their per tri indices. This makes more sense to why we need a custom class for rendering them. 

Two Workflow Ideas : 

* Map each unique vertex position (as particle) too all duplicate vert positions (with unique attribs) per triangle, and update them using the particle positions per tick, while keeping their original other attributes. The drawing would use a standard VAO approach. 
* But as we are going to update Normals anyway, we can just discard all attributes on the input cloth mesh obj, treat each unique vertex position as the particles, and use these to form the indices per tri for the render mesh using an EBO approach. Each of these Particles->Vertices gets its Normals and UVs calculated internally so each particle --> vert is shared, without needing copies of vertices for vertices with different Normals and UVs that then need to be referenced to particles that lie in their same position (at rest) as the above case proposes.

The latter makes sense as unique vertex positions are what should be treated as unique vertices and thus form particles / point masses each, and we don't plan on having fancy normal creasing etc, we re-calc per particle (which then forms each unique / indexed vertices) normals per tick anyway and the UV's can be re-computed based on the rest positions of the particles in local space. Without needing Triangle Soup based copies of vertices (with same positions) just because their other attributes differ. 

So Obj loading we do the normal parsing, but we only store vert positions we can just discard all other attributes (which we need to check for), then when we get to per tri indices we just copy the vert position indices and these indices will map 1:1 with particles, therefore we don't even need to store the vertex positions we can just use them to insatiate particles array whom will be passed to cloth_mesh where the normal and uv coord attributes will be re-added like in VerletClothMesh. So the output will be the particle array and the triangle indices array (which now map to particle indices).

To make things easier as I don't think their gonna try loading arbitrary Obj files when marking I will just assume its using v//vn format. 

Make sure we store a per particle array of the initial state positions so we can "reset to initial state". Also make sure to offset all indices by -1 to account for obj's 1 based indexing. Particles don't need to store an idx as their ID is their position within the particle array (thus based on the original obj unique vert pos indices). We still store IDs because we need them when comparing against other particles, or accessing in alternate iteration orders. 

To save another class we could just make cloth_mesh based within the cloth_state class although I'd be cleaner to separate for the OpenGL state sake, class will be based on primitive class, apart from optimized for updating cloth positions and normal attributes per frame and using indexed drawing based on the passed particles. (Which we then map to a internal indexed vertex array with the correct attribute layout using the calculated attributes per tick for the particles--> now indexed verts to render, using the per tri index list). We can also use the index list back in cloth_state to get edges. 

____

##### Cloth_Mesh Class

As per the internal discussion above, we pass the cloth particles (which like my VerletClothSolver project are just defined as unique vertex positions) to this class, which then calculates the current normals (using neighbouring particles based on index information) this is then serialized into the Vertex Data array (VBO) the Indices only need to be set once to the EBO (when the cloth state is first built as we don't have changing cloth topology here).

Primitive Derivate of new class (based on Primitive ?) : This class can be based on the current Primitive class (its code, not derived), but with optimized updated the positions and normal data and using indexed drawing per particle via tris indices. Or we could try and extend the primitive class via inheritance to define the cloth_mesh class, as its setup quite well to be extended, the render method is virtual so we can implemented indexed drawing, we can make the create_buffers() function virtual and both call the primitive version and then also create the EBOs here internally. We still need the VAO and VBO defined in Primitive anyway, then  can add our cloth specific attribute calculation member functions within this, worth a shot before creating a brand new class based off Primitive's code anyway for code reuse sake. I mean for perf sake we could just write a custom class with no runtime polymorphism / virtual at all, but will try this primitive derived class first. Actually, no I am going to create a custom class, because Primitive depends on passing Vert array to it, whereas we will be passing particles that then need to be serialized into the VAO so it makes more sense just to rewrite it for what we need. 







____

##### Cloth_Solver Class

Springs-Eval will be similar to my original code, but oppose to it been within a Member function of the spring class, it will be within cloth_solver, the spring then directly modifies / adds the resulting force to its two particles which it references. 





____

##### Cloth_Collider Class

Has two components the simulation definition / calculation of the collisions based on some parametric shape, sphere in this primary case, and then the render mesh which needs to match the size based on the parameters passed and used for the collision detection. 

Collision function will eval if some passed particle is within bounds of collider and if so return the signed distance and displacement vector to project out of or the force or impulse to apply. This would be a good class to use polymorphism with this as a virtual function we could support boxes and planes using the same parametric approach. Triangle Mesh based cloth collisions are not really a prio / required at all so probs will leave out for now as this would need acceleration and need parity between render and simulation representations (cannot use implicit or parametric functions to approximate collision bounds). Or could implement SDF Collisions if I had time, that would be fun ! But not gonna happen. 