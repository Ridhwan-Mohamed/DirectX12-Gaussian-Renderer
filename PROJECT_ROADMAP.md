# D3D12 Neural Spatial Viewer

## Project goal

Build a real-time Direct3D 12 viewer for scenes reconstructed with **3D Gaussian Splatting (3DGS)**. The finished project should turn an existing Gaussian scene into an interactive desktop experience, then connect that viewer to a capture pipeline that reconstructs a scene from phone video.

The project combines:

- Modern C++ and Direct3D 12
- Compute shaders and GPU-driven rendering
- 3D reconstruction and camera geometry
- Neural scene representations
- Spatial interaction and an eventual OpenXR path

The first version is a desktop application. A VR headset is not required.

## What the finished demo should show

1. A phone video or public image sequence being converted into a Gaussian scene by an existing reconstruction pipeline.
2. That scene loaded into the custom D3D12 viewer.
3. A smooth fly-through of a detailed reconstructed environment.
4. GPU statistics, visibility counts, sorting time, draw time, and total splat count.
5. Selection, measurement, or semantic highlighting inside the reconstructed scene.
6. A desktop stereo or OpenXR-ready camera mode as a stretch goal.

## Scope decisions

- Use an existing training pipeline. Training a reconstruction model from scratch is outside the initial scope.
- Own the viewer, GPU data layout, culling, ordering, rasterization, interaction, profiling, and optimization.
- Begin with a known-good public `.ply` Gaussian scene before attempting a personal capture.
- Get a correct image before designing large-scene streaming or XR support.
- Keep the application usable in desktop mode throughout the project.

---

## 00 Understand the representation and define the baseline

Read the core parts of the 3DGS paper and inspect a trained Gaussian `.ply` file. Record what each Gaussian stores: position, scale, rotation, opacity, and colour or spherical-harmonic coefficients. Choose one small reference scene and the target PC, resolution, frame rate, and memory budget.

**Ready to move on when**

- The reference scene is available locally.
- Its Gaussian count, file size, attributes, and bounds are recorded.
- You can explain how a Gaussian differs from a point in the LiDAR renderer.
- The target resolution and initial performance goal are written down.

**Learning focus:** neural scene representations, anisotropic covariance, alpha compositing, and the difference between reconstruction and rendering.

## 01 Launch the viewer shell

Create a clean D3D12 application with a resizable window, depth-independent camera controls, shader compilation, validation, frame timing, and safe frames in flight. Reuse proven application infrastructure from the point-cloud renderer where helpful, while keeping the new renderer in its own project.

**Ready to move on when**

- Launch, resize, minimize, restore, and shutdown are stable.
- Graphics validation reports no errors.
- CPU and GPU frame times can be displayed.

**Learning focus:** separating reusable engine infrastructure from renderer-specific code.

## 02 Load and inspect Gaussian scene data

Write the CPU-side scene loader and explicit Gaussian data structures. Parse the reference file, validate every required property, calculate bounds, recentre the scene, and report malformed or unsupported inputs clearly.

Keep the source representation separate from the packed GPU representation so future file formats do not dictate the shader layout.

**Ready to move on when**

- The viewer reports the correct Gaussian count and bounds.
- Attribute ranges and a few decoded records can be inspected.
- Invalid files fail with useful messages.

**Learning focus:** PLY layouts, spherical harmonics, quaternions, scale encoding, opacity encoding, and CPU-to-GPU data contracts.

## 03 Upload a persistent GPU scene

Pack the decoded scene into GPU-friendly buffers and upload it once. Establish clear ownership for the source data, staging resources, and long-lived GPU resources. Release temporary CPU and upload memory after fence completion.

**Ready to move on when**

- The full reference scene remains resident on the GPU.
- Upload resources are released safely.
- GPU memory use and bytes per Gaussian are reported.

**Learning focus:** structured buffers, packing trade-offs, upload lifetime, and GPU memory accounting.

## 04 Render a first correct Gaussian image

Implement the simplest correct splat path. Transform each 3D Gaussian into a screen-space ellipse, evaluate its footprint, and alpha blend it into the image. Correctness matters more than speed here; CPU depth sorting is acceptable for the first reference image.

Add debug modes for Gaussian centres, ellipse bounds, opacity, scale, and depth.

**Ready to move on when**

- The reference scene is recognizable from several viewpoints.
- Ellipse orientation and size respond correctly to the camera.
- Alpha blending does not produce obvious reversed-depth artifacts in the reference view.

**Learning focus:** covariance projection, Jacobians, conic representation, billboard expansion, and front-to-back versus back-to-front compositing.

## 05 Move visibility work onto the GPU

Use a compute shader to reject Gaussians outside the view, behind the camera, below a useful opacity threshold, or too small to affect a pixel. Compact accepted indices into a visible list and generate the count needed by later passes.

**Ready to move on when**

- Visibility changes correctly with camera movement.
- The CPU does not iterate over every Gaussian per frame.
- Source, visible, and rejected counts are measurable.

**Learning focus:** append/compact patterns, atomics, prefix sums, indirect arguments, and conservative screen-space culling.

## 06 Build a GPU ordering pipeline

Generate a camera-depth key for every visible Gaussian and order the visible list on the GPU. Start with a straightforward GPU radix sort or another well-documented approach. Preserve deterministic debug output before optimizing it.

**Ready to move on when**

- Camera movement no longer triggers a CPU sort or GPU readback.
- Transparent overlap remains stable as the camera moves.
- Key generation and sorting have separate GPU timings.

**Learning focus:** radix sorting, key/value buffers, ping-pong resources, barriers, and transparency ordering.

## 07 Make rendering fully GPU driven

Connect visibility, ordering, and drawing without reading counts back to the CPU. Generate indirect draw arguments on the GPU and consume the most recent complete result safely. Introduce double buffering if compute and draw need independent ownership.

**Ready to move on when**

- The per-frame path is GPU cull → GPU order → indirect draw.
- No per-frame point count is copied back to schedule the draw.
- Rapid movement and resize remain validation-clean.

**Learning focus:** `ExecuteIndirect`, resource state transitions, cross-pass synchronization, and producer/consumer buffering.

## 08 Match the visual model

Add spherical-harmonic view-dependent colour, stable opacity evaluation, antialiasing, near-plane handling, and sensible footprint limits. Compare screenshots against a trusted viewer using the same scene and camera.

**Ready to move on when**

- Colour changes correctly with view direction.
- Close, distant, and edge-of-screen splats remain stable.
- Major differences from the reference viewer are understood and documented.

**Learning focus:** spherical harmonics, reconstruction conventions, numerical stability, and image-based validation.

## 09 Profile and optimize the renderer

Measure each GPU pass before changing it. Optimize the largest demonstrated bottleneck, then repeat. Investigate data packing, wave operations, reduced sort work, tiled binning, early rejection, and resolution-aware quality controls.

Create repeatable camera paths and collect frame-time distributions rather than relying only on an FPS counter.

**Ready to move on when**

- Culling, key generation, sorting, rasterization, and total GPU time are reported separately.
- A repeatable benchmark compares at least two scene sizes.
- The renderer holds its chosen target on the development machine or clearly explains the limiting pass.

**Learning focus:** PIX, GPU timestamps, occupancy, bandwidth, overdraw, workload scaling, and evidence-led optimization.

## 10 Connect a real capture pipeline

Record a slow phone video with strong overlap and stable exposure. Use COLMAP or the reconstruction pipeline's integrated camera solver to recover camera poses, then train the scene with an existing 3DGS implementation. Document the conversion into the viewer's accepted format.

The reconstruction tools may remain external programs. The viewer should present the resulting scene and useful metadata cleanly.

**Ready to move on when**

- A scene captured by you renders in the custom viewer.
- Failed captures can be diagnosed using camera tracks and sparse reconstruction output.
- The capture-to-viewer process is reproducible from written steps.

**Learning focus:** camera intrinsics and extrinsics, Structure from Motion, coordinate conversions, capture quality, and the boundary between offline reconstruction and real-time rendering.

## 11 Add spatial interaction and one CV feature

Turn the renderer into a spatial tool rather than only a camera demo. Implement Gaussian picking and world-space measurement first. Then integrate one existing vision model or reconstruction output to attach semantic labels or masks to the scene.

Good first feature: click an object in an input frame, propagate its mask into the reconstructed scene, and highlight the corresponding Gaussians in 3D.

**Ready to move on when**

- The user can select a visible part of the reconstructed scene.
- At least one measurement, annotation, or semantic highlight persists in world space.
- The CV model and real-time viewer communicate through a documented data format.

**Learning focus:** picking translucent primitives, 2D-to-3D association, segmentation, confidence handling, and ML/runtime integration.

## 12 Prepare the viewer for XR without requiring a headset

Separate pose input, view generation, and presentation from desktop controls. Render a stereo pair from simulated eye poses and support OpenXR-compatible coordinate and projection conventions. Add actual OpenXR runtime integration only after desktop stereo is correct.

**Ready to move on when**

- Desktop and simulated stereo modes use the same scene renderer.
- Each eye has a correct view and projection without duplicating scene ownership.
- The renderer has a documented route to an OpenXR swap chain and tracked poses.

**Learning focus:** stereo rendering, pose prediction, coordinate spaces, per-eye views, latency, and the role of OpenXR.

## 13 Package the portfolio demonstration

Create a concise README, architecture diagram, benchmark table, and two-minute demonstration. Show the original capture, sparse camera reconstruction, Gaussian scene, GPU pipeline, semantic interaction, and performance data.

**Ready to call the project complete when**

- A clean checkout builds from documented steps.
- At least one public scene and one personal capture are demonstrated.
- The video explains the technical contribution without requiring prior 3DGS knowledge.
- Performance claims include hardware, resolution, scene size, and measured GPU timings.

**Learning focus:** communicating engineering decisions, reproducibility, and presenting measured results.

---

## Suggested stopping points

### First portfolio checkpoint

Complete milestones 00–04. This proves that you understand the representation and can implement Gaussian rasterization.

### Strong graphics project

Complete milestones 00–09. This demonstrates a fully GPU-driven renderer with real profiling and optimization.

### Graphics + vision project

Complete milestones 00–11. This is the main target and the strongest match for neural graphics, reconstruction, spatial computing, and 3D vision roles.

### XR extension

Complete milestone 12 after the desktop viewer is mature. Physical headset testing is optional until this point.

## Reference material

- [3D Gaussian Splatting for Real-Time Radiance Field Rendering](https://doi.org/10.1145/3592433)
- [Original 3DGS implementation](https://github.com/graphdeco-inria/gaussian-splatting)
- [COLMAP tutorial](https://colmap.github.io/tutorial.html)
- [Khronos OpenXR tutorials](https://github.com/KhronosGroup/OpenXR-Tutorials)
