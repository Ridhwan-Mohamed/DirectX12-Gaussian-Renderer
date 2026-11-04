# Milestone 03: Upload a Persistent GPU Scene

Completed: 2026-11-04

## Outcome

The decoded Gaussian scene is now packed into an explicit renderer-owned
record and uploaded once into a persistent D3D12 default-heap buffer. A
shader-visible structured-buffer SRV describes the resident records for the
first Gaussian pipeline.

## Packed record

Each `GpuGaussian` occupies 240 bytes:

| Field | Bytes | Meaning |
| --- | ---: | --- |
| Recentered position and decoded opacity | 16 | Centre plus peak alpha |
| Decoded scale and padding | 16 | Three positive local standard deviations |
| Normalized quaternion | 16 | Explicit W, X, Y, Z ordering |
| 16 basis-major RGB SH coefficients | 192 | Base and view-dependent colour weights |

Compile-time assertions lock the total size and field offsets so later HLSL
must agree with the CPU layout.

## Reference-scene memory

- Gaussian count: 17,020
- Structured-buffer stride: 240 bytes
- Persistent scene buffer: 4,084,800 bytes
- Persistent scene buffer: approximately 3.90 MiB

The runtime title and Visual Studio debug output report the resident scene size.

## Upload lifetime

```text
decoded GaussianScene
    -> temporary packed CPU vector
    -> CPU-visible D3D12 upload buffer
    -> CopyBufferRegion
    -> GPU default-heap scene buffer
    -> shader-resource state
    -> fence completion
    -> upload buffer released
```

The startup path waits for the copy fence before explicitly releasing the
upload allocation. The temporary packed CPU vector is also scoped to the
upload call and released immediately afterwards. The default-heap buffer
remains owned by `D3D12Renderer` for the viewer's lifetime. This synchronous
wait is intentionally simple and occurs only when replacing the scene, not
once per rendered frame.

## Shader-resource view

The SRV declares:

- `DXGI_FORMAT_UNKNOWN`, because each element is a custom structure;
- 17,020 elements for the reference scene;
- a 240-byte structure stride;
- one shader-visible `CBV_SRV_UAV` descriptor.

Milestone 04 will bind this descriptor through a root signature. A shader will
then index one Gaussian record using its instance or sorted Gaussian index.

## Verification

Automated checks confirm:

- packed count equals decoded count;
- the fixture occupies exactly 4,084,800 bytes;
- position, opacity, scale, WXYZ rotation, DC colour, and a degree-three SH
  coefficient survive packing unchanged;
- the CPU structure is trivially copyable and exactly 240 bytes;
- Debug and Release configurations compile;
- the original PLY fixture and failure-path checks still pass.

Runtime validation confirms:

- the default resource width matches the packed byte count;
- the SRV heap contains one shader-visible descriptor of the expected type;
- the viewer launches with the resident scene size visible in its title;
- the D3D12 debug path remains free of upload and lifetime errors.

## Exit criteria

- [x] The complete reference scene remains resident in a GPU default buffer.
- [x] Source and packed GPU representations remain separate.
- [x] Upload staging is released only after fence completion.
- [x] GPU bytes per Gaussian and total resident scene bytes are reported.
- [x] A structured-buffer SRV is ready for the first Gaussian shader.

Milestone 04 will turn the resident records into projected screen-space
Gaussians and produce the first recognizable image.
