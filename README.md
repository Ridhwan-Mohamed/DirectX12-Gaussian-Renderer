# DirectX 12 Gaussian Renderer

A learning-focused Direct3D 12 viewer for 3D Gaussian Splatting scenes. The
project will grow from a validation-clean desktop shell into a GPU-driven
Gaussian renderer, then connect to personal phone captures and spatial computer
vision experiments.

## Current status

Milestone 01 is in progress. The current executable provides:

- a resizable, DPI-aware Win32 window;
- D3D12 debug-layer support in Debug builds;
- hardware-adapter selection with WARP fallback;
- a triple-buffered flip-model swap chain;
- one command allocator and fence value per frame in flight;
- stable resize, minimize, restore, and shutdown paths;
- a GPU clear pass with timestamp queries;
- CPU/GPU frame timing in the window title;
- a free-fly camera ready for the first render pipeline.

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
assets/scenes/         Small correctness fixtures
PROJECT_ROADMAP.md     Renderer milestones and learning goals
```

## Immediate next step

Add the first shader and pipeline state as a deliberately small lesson:

1. compile a vertex and pixel shader with DXC;
2. define the root signature;
3. define the graphics pipeline state;
4. draw a full-screen or procedural triangle;
5. inspect every field before introducing Gaussian data.
