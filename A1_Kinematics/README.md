#### COMP5823M : Assignment 1 Kinematics 

###### Niall Horn - December 2021 

___

##### Info

Loading `.bvh` motion capture / animation files, and rendering them (using a forward kinematics like approach). The rendering is implemented in Modern OpenGL (4.3) in a compact viewer application that allows for animation playback, scrubbing, stepping and free camera movement. This can be used to view BVH animation files. 

The parser assumes the rotation angles are in Euler (Z,Y,X) order and the only the root joint has translation channels (6 DOF including rotation) where each child joint has 3DOF (rotation only).

The `.bhv` hierarchy is re-built per frame / tick, this is not optimal as it involves depth first traversal using recursion and re-drawing the bone and joint primitives. Could be improved in future to only update transforms of pre-existing bone / skeleton data per tick, based on current set animation frame. 

___

##### Usage

Moving through space can be done using the `W,A,S,D,Q,E` keys, `W,A,S,D` strafe and move the camera, holding `Middle Mouse Button (MMB)` allows the camera to be panned, `Q,E` allows the camera to be raised or dropped along the relative Y Axis. 

`.bvh` files can be written back out via the export function, to some file path, they should match the input file and can be verified by loading into an external animation application such as Blender or Houdini to show that the file format is correct. 

____

##### Building

This project was developed on Windows using MSVC C++ (vc141, Visual Studio 2017 (15.9.19)) with the C++ 14 Language Standard. It should build on Linux fine (when I get round to writing the make file) however the BVH parsing may produce issues with the carriage return `\r` char within the `.bvh` files. 

Make sure the working directory is `/build/bin`/ because the file paths are relative to this directory, for example shaders are loaded via `../../shaders/` directory path. Place `.bvh` files into `/assets/bvh/`, there is currently a bunch of samples here, some have different scales than others. 

Make sure `glew32.dll` and `glfw3.dll` are copied into the `/build/bin/` directory for runtime linking. They should be in this directory by default. (Replace with post-build script)

**ToDo:** CMake 

___

##### Dependencies

* **GLM** - For Linear Algebra / transformation operations
* **GLEW** - OpenGL Function loading. *Ideally eliminate this dependency if time*. 
* **GLFW** - OpenGL Context, Window and Input handling. 
* **DearImGui** - GUI (via GLFW and OpenGL Implementation backends).
* **Stb Image** - Texture image file loading (jpg, png, tiff, etc). 

You will of course need to link with OpenGL `opengl32.lib` on Windows or `-lopengl32` on Linux. 

___

##### References 

* **BVH Format Guide** : *https://research.cs.wisc.edu/graphics/Courses/cs-838-1999/Jeff/BVH.html*

* **BVH reference code** :  *Masaki OSHITA (www.oshita-lab.org)*
