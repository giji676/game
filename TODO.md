# TODO:
1. Figgure out storing and freeing of assets loaded with gj-lib
2. Rename renderer/debug to debug_renderer for consistency
3. For asset_manager .get<> functions add a more graceful termination when
    object being retreived doesn't exist. Print the error atleast.

# Features:
1. UI system
2. Editor UI
3. Improved shaders
4. Physics
5. Collisions
6. Animations
7. Audio
8. Navigation
9. Networking
10. Live script loading
11. LOD

*wip* = Work In Progress / Early Stages / Very Basic

# Completed features:
1. Asset loading
2. Scene graph
3. Materials
4. Profiler (wip)
5. Debug renderer
6. Renderer
7. Script system
8. Raycasting
9. Input system (wip)

# TODO dumps:
1. Precompute normal matrix — transpose(inverse(model)) is computed per vertex in the shader right now. Add a second SSBO with precomputed normal matrices, update it alongside the transform SSBO each frame
2. Texture atlasing — you have 79 material groups meaning 79 glMultiDrawElementsIndirect calls. If you atlas textures you could reduce to 1 shader group = 1 draw call total
3. Skip sort when scene is static — if no objects changed material/shader this frame, sortedIndices is identical to last frame. Dirty flag to skip the 8ms sort
4. Parallel collectRenderCommands — traverse scene graph on multiple threads with std::for_each + execution policy
5. GPU frustum culling — move the frustum test to a compute shader that writes the indirect buffer directly, CPU does zero per-object work
