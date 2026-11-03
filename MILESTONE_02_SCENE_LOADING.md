# Milestone 02: Load and Inspect Gaussian Scene Data

Completed: 2026-11-03

## Outcome

The viewer now loads the reference Inria-style Gaussian PLY into an explicit,
decoded CPU representation before D3D12 initialization. The source file layout
is confined to the loader and does not dictate the future GPU buffer layout.

## Data path

```text
binary PLY bytes
    -> parsed element/property schema
    -> property-name lookup
    -> finite source scalar values
    -> decoded scale, opacity, quaternion, and SH coefficients
    -> recentered GaussianScene
```

## Runtime representation

Each `Gaussian` contains:

- a position recentered around the source bounds centre;
- three positive decoded scales;
- a normalized quaternion with explicit W, X, Y, Z field names;
- decoded opacity in `[0, 1]`;
- 16 basis-major RGB spherical-harmonic coefficients.

The loader converts the PLY's channel-major `f_rest_0..44` sequence into
basis-major RGB values so a future shader can read one RGB coefficient per SH
basis term.

## Validation behaviour

The loader rejects:

- missing or invalid PLY magic and format declarations;
- ASCII and big-endian PLY data;
- a missing or empty vertex element;
- duplicate or missing required Gaussian properties;
- list properties inside the Gaussian record;
- truncated records;
- non-finite and out-of-range values;
- scales that cannot be decoded to positive finite values;
- zero or invalid quaternions;
- unreasonable record counts.

Errors include the scene path and the failed property or record where relevant.

## Verified reference result

- Gaussian count: 17,020
- Bounds minimum: `(-0.494141757, -0.190222681, -0.116828270)`
- Bounds maximum: `(0.493660569, 0.192169592, 0.096737459)`
- Recenter offset: `(-0.000240594, 0.000973456, -0.010045405)`
- Decoded opacity range: `[0.003616542, 0.999995470]`
- Raw quaternion norm range: `[0.424695462, 1.946974039]`

The fixture test also checks the first decoded Gaussian, normalized quaternion
length, positive scales, unsupported ASCII input, and a missing-property error.

## Integration

- The bundled fixture is copied beside the executable after every build.
- With no argument, the viewer loads that copied fixture.
- A different compatible PLY can be supplied as the first command-line argument.
- The full inspection report is sent to the Visual Studio debug output.
- The live window title displays the loaded Gaussian count.

## Exit criteria

- [x] The viewer reports the correct Gaussian count and bounds.
- [x] Attribute ranges and decoded records can be inspected.
- [x] Invalid and unsupported files fail with useful messages.
- [x] Source layout is separated from the future packed GPU representation.
- [x] Debug and Release builds succeed.
- [x] Automated fixture and failure-path checks pass.

Milestone 03 will define the packed GPU record, upload it once into a persistent
structured buffer, and report bytes per Gaussian and total resident scene size.
