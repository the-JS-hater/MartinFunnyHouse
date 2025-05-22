#align(center)[
  = Cubemap Mirrors
  #grid(
    columns: 2,
    gutter: 20pt,
    [
      Anton Nilsson \
      antni004\@student.liu.se
    ],
    [
      Morgan Nordberg \
      morno368\@student.liu.se
    ],
  )
]

== Introduction
The project goal was to create a 3D scene, where the "player" can walk around as a 3D model in a first-person-perspective; as well as to model mirrors that reflect the models in the world. 
Additional goals were to gradually improve the mirror implementation using more and more advanced techniques. 
Yet another goal was to make it possible to have multiple mirrors that can reflect each other.

== Background Information
Some basic datastructures needed for the overall discussion are explained here.

=== Cubemap Texture
A cubemap consists of 6 textures that make up the 6 sides of the cube, illustrated in @cubemap-illustration.
Built-in support for working with cubemap textures exists directly in OpenGL and GLSL.
The cubemap texture can for example be sampled using a 3D vector.

#figure(
  image("images/cubemap_illustration2.png", width: 65%),
  caption: [Cubemap.],
) <cubemap-illustration>

=== FrameBufferObject
A FrameBufferObject (FBO) is an object in OpenGL which allows us to render the scene to a texture, instead of rendering it to the screen. 
It has its own texture and various buffers, such as a depth buffer. 
In our case it's useful for capturing the scene from various perspectives and positions before rendering to the screen.

== Implementation Details
Here we will go into more detail on how various techniques were implemented as well as overall project structure.

=== Project Structure
The project structure contains all source code in `src/main.cpp`, all 3D models in `models/`, textures in `textures/` and shader files in the `shaders/` directory. 
The various dependencies are contained within the `common/` directory.

=== Dependencies 
We used no additional dependencies besides those provided for the lab assignments. 
We did however use OpenGL 4.6 in our shaders.

=== Pre-rendered Cubemap
In OpenGL there is a built-in type for dealing with cubemap textures in a shader called "samplerCube". 
In the shader you pass a uniform samplerCube which is sampled using a vector. 
This vector is found by reflecting the directional vector from the observer to the mirror's surface w.r.t the normal vector of the surface.
The result of using a pre-rendered cubemap can be seen in @prerendered.

The problem with approach is that the cubemap can't account for changes in the enviorment and thus models in the scene won't be reflected. 
There is also issues with scale/perspective because the sample vector is only a good approximation if the enviorment is an infinitley large cube. 
In a small scene this will cause very apparent distortions in perspective in the mirror. 
The first problem is solved by using dynamic cubemaps, and the latter problem is solved by parallax correction. 
Both of these techniques are covered in later sections of the report.

#figure(image("images/prerendered_cubemap.png", width: 65%), caption: [Pre-rendered cubemap.]) <prerendered>

=== Dynamic Cubemap
Dynamic cubemaps work by giving each mirror it's own FBO, that contains a cubemap texture. 
The scene can then be rendered from the mirrors position, in the positive and negative x, y and z directions.
The result of these renders is saved in the cubemap texture of the mirror's associated FBO. 
This texture is then used for sampling when rays are reflected on the mirrors surface as described in the pre-rendered cubemap section. 
This makes the various models and changes in the scene visible in the mirrors reflections.
@dynamic-cubemap shows the scene being reflected in the mirror using a dynamic cubemap.

#figure(
  image("images/dynamic_cubemap.png", width: 65%),
  caption: [Dynamic
    cubemap.],
) <dynamic-cubemap>

=== Basic Bumpmap
Using the sinus function on the surface position can distort the normal vectors to create fun affects like those found in "fun houses" at amusement parks. 
This effect is shown in @bumpmap. 
A problem with this technique is that bumps in a real mirror could cause the mirror to reflect itself, as illustrated in @internal-bounces. 
Because of limitations with how the dynamic cubemaps are created, this effect can't be replicated. 
The mirror will also never be able to reflect itself since the actual mirror model is flat.

#figure(
  image("images/bumpmap.png", width: 65%),
  caption: [Normal vectors distorted.],
) <bumpmap>

#figure(
  image("images/internal_bounces2.png", height: 20%),
  caption: [Internal bounces.],
) <internal-bounces>

=== Parallax Correction
As said using cubemaps for reflections comes with the drawback that the reflection can become very distorted as they are only approximations. 
This distortion is a parallax issue which is a result of sampling using the reflection vector. 
The cause is that the position of the cubemap is not the same as the position of the reflection point.

The solution to this parallax issue is using parallax correction to get the vector which samples the cubemap. The difference between the sample vectors is illustrated in @cubemap-distortion. 
The difference this makes on the reflections is showcased in @parallax-comparison. 
The idea is to define an axis aligned bounding box with dimensions slightly larger than the size of the room/scene. 
The sampling vector can then be found by finding the intersection point between reflection vector from the reflection point and the AABB.

#figure(
  image("images/cubemap_distortion2.png", width: 65%),
  caption: [Normal sampling on the left; parallax corrected on the right.],
) <cubemap-distortion>

#figure(
  grid(
    columns: 2,
    [
      #image("images/cubemap_error.png", width: 95%)
    ],
    [
      #image("images/cubemap_corrected.png", width: 95%)
    ],
  ),
  caption: [Normal cubemap on the left; parallax corrected cubemap on the right.],
) <parallax-comparison>

Define $p_0 = (x_0, y_0, z_0)$ as the reflection point, $p = (x, y, z)$ as the intersection point with the AABB and $d = (x_d, y_d, z_d)$ as the reflection direction/vector. 
Also define $b_min = (x_min, y_min, z_min)$ as the corner of the AABB with the smallest x, y and z components and $b_max = (x_max, y_max, z_max)$ as the corner with the largest components. 
The idea to find the intersection point with the AABB is to handle each axis separately. 
Calculating if the ray $p_0 + d t$ intersects with the $x_min$ or $x_max$ plane and the distance to that plane can be done as follows:
\
\
#align(center)[
  $
  x_0 + x_d t_x_min = x_min => t_x_min = (x_min - x_r) / x_d & \
  t_x_max = (x_max - x_r) / x_d & \
  t_x = max {t_x_min, t_x_max}
  $
]
\
The scalars $t_x_min$ and $t_x_max$ are the scalars of $d$ used to get $x_min$ and $x_max$, respectivly. 
The positive of the two scalars will be the scalar $t_x$ giving the intersection point of the AABB in the x-axis, that is plane in the x direction of the reflection vector $d$. 
Similarly this can be done to find $t_y$ and $t_z$. 
The intersection point with the AABB is then given by the minimum of these three (since it will give the first plane intersection) and solving for $p$:
\
\
#align(center)[
  $
  t = min {t_x, t_y, t_z} \
  p = p_0 + d t
  $
]
\
=== Recursive Mirrors
If there is multiple mirrors in a scene they won't be able to reflect eachother correctly when using only one FBO for each mirror. 
Since one mirror will render its cubemap texture before the other there will be visible artifacts. 
One idea to solve this is to use two FBO:s per mirror, which are switched between each frame. 
This will solve the problem by using the cubemap texture from the previous frame when rendering the cubemap texture for the current frame. 
Each recursive reflection step will lag one frame behind, but after a while the changes propagate through the reflections.
These recursive mirrors also makes the issue of not parallax correcting the sample vector apparent. 
The other mirror will appear larger than it should, which causes an unwanted effect. 
When the sample vector is corrected the resulting reflection looks more like expected, as seen in @recursive_mirror.

#figure(
  grid(
    columns: 2,
    [
      #image("images/recursive_mirror.png", width: 95%)
    ],
    [
      #image("images/parallax_recursive.png", width: 95%)
    ],
  ),
  caption: [Comparison of recursive mirror with and whitout parallax correction],
) <recursive_mirror>

== Problems

Most of the problems encountered such as the cubemap parallax error has been explained before.
Outside of these intrinsic issues with the solutions we chose the most common problems encountered were misunderstanding of how OpenGL works.
One such is example is that we forgot to change the framebuffer used before trying to render to its texture.

== Conclusion

In conclusion there exists a lot of different ways to do reflections in computer graphics.
Using cubemaps has both both advantages, drawbacks and limitations.
The techniques used is useful for example when supporting hardware without raytracing accelerators.
Dynamic cubemaps can however also quickly become expensive.
In the recursive mirror example the scene already had to be rendered 13 times per frame when having just two mirrors.

Cubemaps also becomes visibly distorted when looking at a reflection on a flat surface.
Using parallax correction helps a lot with the distortions caused by using cubemaps.
Since the trick used to find the parallax corrected vector requires an AABB it can however be quite limiting.
The math could probably be changed to support oriented bounding boxes. 
These will however also just be approximations of the scene/room if it isn't just a flat box.

As modern GPU:s receive more and more raytracing cores these techniques are less needed.
Cubemaps are still however an interesting way of faking reflections efficiently.
The techniques are also as said still useful when supporting older hardware.

== Source Code
#show link: underline
The code for the project is available in
an open GitHub repo: #link("https://github.com/the-JS-hater/MartinFunnyHouse")

