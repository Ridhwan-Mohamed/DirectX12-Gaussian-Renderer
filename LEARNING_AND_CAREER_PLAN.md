# Spatial Computing Learning and Career Plan

## Direction

The target is to become a **real-time 3D systems engineer moving toward spatial computing and reconstruction**.

This combines:

- Modern C++ and GPU programming
- Real-time graphics and visualization
- Camera geometry and 3D reconstruction
- Applied computer vision and machine learning
- Eventually, XR interaction and OpenXR

The goal is not to become an AI research scientist before applying for jobs. The goal is to understand vision and machine learning well enough to integrate useful algorithms into fast, reliable 3D applications.

Potential job titles include:

- 3D Software Engineer
- Computer Vision Software Developer
- Reconstruction Engineer
- Spatial Computing Engineer
- Mapping or Perception Engineer
- GPU Engineer for computer vision
- Simulation or Visualization Engineer
- XR Runtime Engineer

The professional pitch is:

> I can take spatial data or research algorithms and turn them into interactive, GPU-accelerated software.

## Current foundation

The D3D12 LiDAR renderer already provides relevant experience with:

- Vectors, matrices, cameras, projection, and coordinate spaces
- Point clouds, octrees, spatial selection, and level of detail
- Compute shaders, indirect drawing, and GPU profiling
- Asynchronous streaming and memory budgeting
- Modern C++ application and resource lifetime management
- Reading a research paper and implementing its technique

This means the learning path can begin with camera geometry and applied vision rather than repeating basic 3D programming.

## Expected timeframe

Assuming approximately **8 to 12 focused hours per week**:

| Time invested | Expected capability |
| --- | --- |
| 2–4 weeks | Use pretrained vision models and understand a basic PyTorch inference workflow |
| 2–3 months | Build camera calibration, feature matching, pose, registration, and simple reconstruction experiments |
| 4–6 months | Produce a strong project combining vision, AI, reconstruction, and real-time 3D |
| 6–12 months | Become a credible candidate for junior spatial computing, applied computer vision, and 3D vision roles |
| 2+ years | Develop research-level depth in an area such as SLAM, neural reconstruction, or learned rendering |

These are not waiting periods before applying. Job applications, interview preparation, and learning should happen in parallel.

## Learning track A: camera geometry and classical 3D vision

Spend roughly four to six weeks becoming comfortable with:

1. The pinhole camera model
2. Camera intrinsics and extrinsics
3. Perspective projection and coordinate conversions
4. Lens distortion and camera calibration
5. Feature detection, description, and matching
6. Epipolar geometry and the essential and fundamental matrices
7. PnP camera-pose estimation
8. Triangulation
9. Point-cloud registration and ICP
10. The purpose of bundle adjustment

The required mathematics is primarily linear algebra, geometry, basic probability, and an introduction to nonlinear optimization. The practical target is to understand each algorithm's inputs, outputs, assumptions, failure cases, and role in a reconstruction pipeline. Deriving every method from scratch is not required.

### Small exercises

- Calibrate a real phone or webcam with OpenCV.
- Visualize the recovered camera axes and frustum.
- Match features between two images and reject bad matches.
- Estimate the relative camera pose between two views.
- Triangulate a small set of points.
- Align two point clouds with ICP and display the before and after result.

## Learning track B: modern machine learning

Previous TensorFlow experience provides a useful starting point. Learn enough PyTorch to:

- Create and manipulate tensors on the CPU and GPU
- Understand a model's forward pass, loss, and backpropagation
- Train and evaluate a small model
- Use a pretrained model for inference
- Understand normalization, tensor layouts, checkpoints, and device placement
- Read and modify an unfamiliar research repository
- Diagnose shape, memory, and input-format problems
- Export results into a C++ or real-time application

Do not spend months training generic image classifiers. Use small exercises to understand the mechanics, then apply existing models to spatial problems.

### Useful applied model categories

- Monocular depth estimation
- Semantic and instance segmentation
- Object detection and tracking
- Image feature matching
- Camera-pose estimation
- Visual place recognition

## Learning track C: spatial AI and reconstruction

After the foundations, study how the pieces form a spatial system:

- Structure from Motion
- Visual odometry and SLAM
- Multiview stereo
- Neural depth and feature matching
- NeRFs and 3D Gaussian Splatting
- Projecting 2D masks and labels into a reconstructed 3D scene
- Confidence, temporal consistency, and reconstruction failure analysis

The neural spatial viewer is the main practical vehicle for this learning.

## Target project pipeline

```text
Phone video or public image sequence
                ↓
Camera calibration and recovered poses
                ↓
Sparse reconstruction
                ↓
Gaussian or point-based scene reconstruction
                ↓
Pretrained segmentation or depth model
                ↓
Labels and observations associated with the 3D scene
                ↓
Interactive D3D12 viewer
```

The final application should eventually allow a user to:

- Import a reconstructed scene from a phone capture
- Inspect the recovered camera path
- Navigate the scene interactively
- Select reconstructed objects or regions
- Measure distances or place persistent annotations
- Highlight a semantic class or segmented object in 3D
- Inspect reconstruction confidence and failure cases

The detailed renderer milestones are in [PROJECT_ROADMAP.md](PROJECT_ROADMAP.md).

## How the learning tracks map onto the project

| Project milestones | Parallel learning |
| --- | --- |
| 00–04 | 3DGS representation, covariance, projection, and PyTorch inference basics |
| 05–09 | GPU algorithms, profiling, ordering, compaction, and production renderer design |
| 10 | Calibration, camera poses, feature matching, Structure from Motion, and reconstruction diagnosis |
| 11 | Segmentation or depth inference, 2D-to-3D association, selection, and spatial annotation |
| 12 | Stereo rendering, pose input, OpenXR coordinate conventions, and XR latency concepts |

## XR path without a headset

A headset is not required for the early project:

1. Build the reconstruction and viewer on desktop.
2. Keep camera pose input separate from rendering logic.
3. Add simulated left-eye and right-eye views.
4. Validate OpenXR-compatible coordinate and projection conventions.
5. Add the OpenXR runtime and physical headset testing after the desktop renderer is mature.

Actual hardware will eventually be necessary to evaluate head tracking, interaction, stereo comfort, scale, motion-to-photon latency, and spatial anchors. It is not necessary for learning the underlying rendering and reconstruction systems.

## Career execution alongside the project

### Immediate priority

Prepare thoroughly for the 2K graduate interview and continue studying Lyra. Professional C++ and Unreal experience would be a strong entry point even if the role is not exclusively about graphics.

### Application strategy

Search beyond the title `Graphics Programmer`. Include:

- Real-time 3D
- Computer vision
- 3D reconstruction
- Mapping and localization
- Perception
- Simulation and visualization
- Spatial computing
- GPU computing
- XR runtimes and SDKs

Apply to graduate programs, Developer I or II positions, associate roles, fixed-term R&D jobs, and university research software positions. Requirements around one or two years of experience can still be worth challenging when the technical overlap is strong.

### Portfolio checkpoints

1. **Correct Gaussian renderer:** prove understanding of the representation and rasterization.
2. **GPU-driven renderer:** demonstrate compute, sorting, indirect drawing, profiling, and optimization.
3. **Personal reconstruction:** demonstrate calibration, camera recovery, capture quality, and data conversion.
4. **Spatial CV feature:** demonstrate that vision results can drive useful interaction inside the 3D world.
5. **XR-ready architecture:** demonstrate stereo rendering and clean pose/view separation.

Each checkpoint should produce a short video, measured results, and a concise technical explanation. These are opportunities to apply for relevant positions, not prerequisites for beginning the job search.

## Main scope guardrails

- Use existing reconstruction and pretrained model pipelines initially.
- Own the D3D12 viewer, GPU pipeline, integration, interaction, profiling, and optimization.
- Learn algorithms by using and inspecting them before attempting original research.
- Add one meaningful CV capability rather than collecting many shallow model demos.
- Keep the application working on desktop before expanding into XR.
- Prioritize measured engineering results over chasing whichever AI technique is newest.

## Definition of success

This plan succeeds when the portfolio demonstrates that spatial data can travel through an understandable end-to-end pipeline: capture, camera recovery, reconstruction, GPU rendering, vision-assisted interpretation, and real-time interaction.

The desired outcome is a credible identity as a software engineer who can work across graphics, GPU systems, reconstruction, and applied computer vision—with a clear path into XR when hardware becomes available.
