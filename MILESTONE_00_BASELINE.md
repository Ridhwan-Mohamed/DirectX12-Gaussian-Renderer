# Milestone 00: Representation and Baseline

Completed: 2026-10-30

## Outcome

The project has a small, locally available Inria-style 3D Gaussian Splatting
scene. Its schema and numerical ranges have been inspected, and the initial
desktop performance and memory targets are defined.

## Reference scene

- Local file: `assets/scenes/anchor-splat-demo/point_cloud.ply`
- Source: `https://github.com/zhude233/AnchorSplat/blob/main/examples/3dgs_sr_demo.ply`
- Source description: runnable 3DGS-SR test sample in Inria-style 3DGS format
- Source license: MIT
- File size: 4,018,196 bytes (3.83 MiB)
- SHA-256: `381710548C510395AD97FB652B17ECFFCBED4B66029CFD6A00DB6E8EF513ABD4`
- Encoding: binary little-endian PLY 1.0
- Gaussian count: 17,020
- Header size: 1,476 bytes
- Source record size: 236 bytes per Gaussian
- Scalar format: 59 little-endian `float32` properties per Gaussian
- Non-finite values found: 0

This is intentionally a very small baseline scene. It is suitable for loader,
upload, projection, and rasterization correctness work. A larger standard scene
will be added later for performance evaluation; it should not replace this
fast-loading correctness fixture.

## Source record schema

| Property group | Count | Meaning | Runtime decoding |
| --- | ---: | --- | --- |
| `x`, `y`, `z` | 3 | Gaussian centre in scene coordinates | Use directly, then apply any scene transform |
| `f_dc_0..2` | 3 | Degree-0 spherical-harmonic colour coefficients | Combine with the constant SH basis term and `+0.5` |
| `f_rest_0..44` | 45 | Remaining degree-1 through degree-3 SH colour coefficients | Evaluate against the normalized view direction |
| `opacity` | 1 | Opacity logit | `alpha = sigmoid(opacity)` |
| `scale_0..2` | 3 | Natural logarithm of the three local Gaussian scales | `scale = exp(storedScale)` |
| `rot_0..3` | 4 | Quaternion components, using the Inria convention | Normalize before constructing a rotation matrix |

Total: 59 floats = 236 bytes per source record.

The original GraphDeco writer can also include `nx`, `ny`, and `nz` dummy
normal fields. They are not present in this reference file. The future loader
must therefore identify properties by name and validate required semantics
rather than reinterpret the payload as one hard-coded C++ struct.

## Spherical-harmonic colour layout

The scene uses degree-3 spherical harmonics. Degree 3 has
`(3 + 1)^2 = 16` coefficients per colour channel:

- 1 degree-0/DC coefficient per channel, stored in `f_dc_0..2`
- 15 higher-order coefficients per channel
- 3 channels x 15 higher-order coefficients = 45 `f_rest_*` values
- 48 total colour coefficients per Gaussian

The `f_rest_*` values use the common Inria channel-major layout: the 15
non-DC coefficients for red, then green, then blue.

For a view direction `d`, rendering conceptually evaluates:

```text
colour(d) = 0.5 + sum(coefficient[i] * SHBasis[i](d))
```

The degree-0 term supplies a view-independent base colour. Higher-degree terms
make the colour vary smoothly with viewing direction. They compactly fit effects
such as highlights and other view-dependent appearance; they are not a physical
material or lighting model.

For a DC-only preview, the conversion constant is approximately
`0.28209479177387814`:

```text
baseRGB = 0.5 + 0.28209479177387814 * float3(f_dc_0, f_dc_1, f_dc_2)
```

The final pixel contribution also includes the Gaussian footprint and decoded
opacity, followed by ordered alpha compositing with other Gaussians.

## Measured scene bounds

| Measurement | X | Y | Z |
| --- | ---: | ---: | ---: |
| Minimum | -0.494141757 | -0.190222681 | -0.116828270 |
| Maximum | 0.493660569 | 0.192169592 | 0.096737459 |
| Extent | 0.987802327 | 0.382392272 | 0.213565730 |
| Bounds centre | -0.000240594 | 0.000973456 | -0.010045405 |

The PLY does not declare a physical unit. These are scene-coordinate units and
must not be described as metres without external metadata.

## Measured decoded attribute ranges

### Scale after `exp`

| Local axis | Minimum | Median | 99th percentile | Maximum |
| --- | ---: | ---: | ---: | ---: |
| 0 | 0.000000081 | 0.002050338 | 0.028215285 | 0.072709062 |
| 1 | 0.000000296 | 0.001220425 | 0.025623802 | 0.066799722 |
| 2 | 0.000002237 | 0.005408444 | 0.033169097 | 0.077256674 |

### Opacity after `sigmoid`

- Minimum: 0.003616543
- 1st percentile: 0.011134365
- Median: 0.493405973
- Mean: 0.519169464
- 99th percentile: 0.998887941
- Maximum: 0.999995515
- Gaussians below 0.01 opacity: 151
- Gaussians above 0.99 opacity: 1,879

### Quaternion input quality

- Raw norm range: 0.424695467 to 1.946974100
- Median raw norm: 1.139630460
- Zero or near-zero quaternions: 0
- Records farther than 0.001 from unit length: 16,990

Quaternion normalization is therefore a required decode step, not merely a
defensive fallback.

### Colour data

- DC-only RGB is derived with `0.5 + SH_C0 * f_dc` before display clamping.
- 1,054 DC-only channel components fall outside `[0, 1]` before clamping.
- Higher-order SH coefficient range: -0.416834176 to 0.393014878
- Mean absolute higher-order coefficient: 0.038208268
- 99th percentile absolute higher-order coefficient: 0.158461099

Colour evaluation must use floating-point values and apply the reference
renderer's output clamping convention after SH evaluation.

## Example decoded record

The first Gaussian demonstrates the difference between stored and usable values:

```text
position          = (-0.01195582,  0.00894820, 0.03385466)
stored log scale  = (-3.54261279, -3.90573335, -3.59316778)
decoded scale     = ( 0.02893762,  0.02012619,  0.02751104)
stored opacity    = -5.54774570
decoded opacity   =  0.00388111
raw quaternion    = (0.80017519, 0.70971501, 0.50038809, 0.64327997)
raw quat norm     = 1.34468327
DC-only RGB       = (0.45886066, 0.36504564, 0.26077447)
```

## Gaussian versus LiDAR point

A LiDAR point is primarily a discrete measurement at a position. Its rendered
size is usually chosen by the renderer. A 3D Gaussian is a learned continuous
primitive: its position, anisotropic extent, orientation, opacity, and angular
colour function are scene data. Its 3D covariance projects to an oriented 2D
ellipse, and its translucent contribution must be composited with other
Gaussians in a meaningful depth order.

## Reconstruction versus rendering

Reconstruction optimizes the Gaussian parameters so that renders from known
training cameras agree with the captured images. The viewer consumes the
resulting explicit Gaussian records. It does not need to run a neural network
per pixel, although it still needs substantial GPU algorithms for projection,
visibility, SH evaluation, ordering, rasterization, and alpha compositing.

## Initial target platform and budget

- CPU: Intel Core i7-1360P
- GPU: Intel Iris Xe Graphics, driver 32.0.101.7085
- System memory: 15.7 GiB
- Display: 1920 x 1200
- Initial internal rendering resolution: 1280 x 720
- Initial correctness target: stable 30 FPS (33.3 ms total frame time)
- Initial GPU-resident scene and working-buffer budget: 512 MiB
- Desired packed Gaussian scene share of that budget: no more than 256 MiB
- Initial reference-scene source payload: 3.83 MiB

The 512 MiB figure is a conservative project budget for an integrated GPU with
shared memory, not a claim about dedicated VRAM. It will be revised using DXGI
memory-budget telemetry and measured pass timings once the viewer shell exists.

## Exit criteria

- [x] A small reference scene is available locally.
- [x] Gaussian count, file size, schema, bounds, and attribute ranges are recorded.
- [x] The difference between a Gaussian and a LiDAR point is documented.
- [x] Neural scene representation, anisotropic scale/covariance, view-dependent
      colour, alpha compositing, and reconstruction versus rendering are understood
      at the level required to begin the viewer.
- [x] Target hardware, resolution, frame rate, and initial memory budget are written down.

Milestone 00 is complete. Milestone 01 should preserve this file as the first
loader/rasterizer correctness fixture.
