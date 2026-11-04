# DirectX 12 Gaussian Renderer

A learning-focused Direct3D 12 viewer for 3D Gaussian Splatting scenes. The
project will grow from a validation-clean desktop shell into a GPU-driven
Gaussian renderer, then connect to personal phone captures and spatial computer
vision experiments.

## Current status

Milestones 01 through 03 are complete. The current executable provides:

- a resizable, DPI-aware Win32 window;
- D3D12 debug-layer support in Debug builds;
- hardware-adapter selection with WARP fallback;
- a triple-buffered flip-model swap chain;
- one command allocator and fence value per frame in flight;
- stable resize, minimize, restore, and shutdown paths;
- a GPU clear pass with timestamp queries;
- CPU/GPU frame timing in the window title;
- a free-fly camera ready for the first render pipeline;
- property-by-name binary little-endian PLY loading;
- scale, opacity, quaternion, and SH-layout decoding;
- source bounds, scene recentering, diagnostics, and fixture tests;
- an explicit 240-byte CPU-to-GPU Gaussian record;
- a one-time upload into a persistent default-heap structured buffer;
- a shader-visible SRV for all resident Gaussians;
- reported Gaussian count, stride, and resident scene-buffer size.

There is intentionally no Gaussian shader yet. The first HLSL and graphics
pipeline state will be added as a dedicated, explained milestone.

## Build

Requirements:

- Windows 10 or 11
- Visual Studio 2022 with **Desktop development with C++**
- Windows 10/11 SDK

Open the repository folder in Visual Studio and select the `vs2022-x64`
configure preset, or build from a Developer PowerShell:

```powershell
cmake --preset vs2022-x64
cmake --build --preset debug
```

The executable is generated under:

```text
out/build/vs2022-x64/Debug/GaussianRenderer.exe
```

The bundled fixture loads automatically. To inspect another compatible Inria
3DGS file, pass its path as the first argument:

```powershell
./out/build/vs2022-x64/Debug/GaussianRenderer.exe "C:/scenes/example/point_cloud.ply"
```

Run the loader checks with:

```powershell
ctest --test-dir out/build/vs2022-x64 -C Debug --output-on-failure
```

## Controls

| Input | Action |
| --- | --- |
| `W A S D` | Move horizontally relative to the camera |
| `Q / E` | Move down / up |
| Hold right mouse button | Look around |
| `Shift` | Move faster |
| `V` | Toggle vertical synchronization |
| `Esc` | Exit |

## Layout

```text
src/Application.*      Win32 lifecycle, input, and main loop
src/D3D12Renderer.*    D3D12 device, swap chain, commands, fences, and timing
src/Camera.*           View/projection state and free-fly movement
src/GaussianScene.*    Decoded CPU-side scene representation and diagnostics
src/PlyGaussianLoader.* Property-driven PLY parsing and validation
src/GpuGaussian.*      Explicit packed CPU-to-GPU scene contract
assets/scenes/         Small correctness fixtures
PROJECT_ROADMAP.md     Renderer milestones and learning goals
```

## Immediate next step

Milestone 04 adds the first Gaussian graphics pipeline as a focused lesson:

1. compile the first Gaussian vertex and pixel shaders with DXC;
2. define the root signature and graphics pipeline state;
3. generate one quad per Gaussian with `SV_VertexID` and `SV_InstanceID`;
4. project covariance and evaluate elliptical falloff;
5. sort on the CPU and alpha blend the first recognizable image.
